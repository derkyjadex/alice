/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <SDL2/SDL.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>

#include "alice/host.h"
#include "albase/geometry.h"
#include "graphics.h"
#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"
#include "widget_internal.h"
#include "albase/wrapper.h"
#include "albase/fs.h"

struct AlHost {
	lua_State *lua;
	bool finished;

	Vec2 screenSize;

	AlWidget *root;
	AlWidget *grabbingWidget;
	AlWidget *keyboardWidget;
};

static AlLuaKey hostKey;
static int luaopen_host(lua_State *L);

AlError al_host_systems_init()
{
	BEGIN()

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		al_log_error("Couldn't initialize SDL: %s", SDL_GetError());
		THROW(AL_ERROR_GRAPHICS)
	}

	TRY(graphics_system_init());

#if defined(__APPLE__)
	const char *locale = "UTF-8";
#else
	const char *locale = "C.UTF-8";
#endif
	char *result = setlocale(LC_CTYPE, locale);
	if (!result || strcmp(locale, result)) {
		al_log_error("Could not set locale to %s", locale);
		THROW(AL_ERROR_GENERIC);
	}

	TRY(al_fs_chdir_app_data());

	CATCH(
		al_host_systems_free();
	)
	FINALLY()
}

void al_host_systems_free()
{
	graphics_system_free();
	SDL_Quit();
}

static AlError run_alice_scripts(lua_State *L)
{
	BEGIN()

	TRY(AL_SCRIPT_RUN(L, widget));
	TRY(AL_SCRIPT_RUN(L, draggable));
	TRY(AL_SCRIPT_RUN(L, toolbar));
	TRY(AL_SCRIPT_RUN(L, slider_widget));
	TRY(AL_SCRIPT_RUN(L, colour_widget));
	TRY(AL_SCRIPT_RUN(L, model_widget));
	TRY(AL_SCRIPT_RUN(L, file_widget));
	TRY(AL_SCRIPT_RUN(L, text_box));
	TRY(AL_SCRIPT_RUN(L, panning_widget));
	TRY(AL_SCRIPT_RUN(L, model_view_model));

	PASS()
}

AlError al_host_init(AlHost **result)
{
	BEGIN()

	AlHost *host = NULL;
	TRY(al_malloc(&host, sizeof(AlHost)));

	host->lua = NULL;
	host->finished = false;
	host->screenSize = (Vec2){1, 1};
	host->root = NULL;
	host->grabbingWidget = NULL;
	host->keyboardWidget = NULL;

	host->screenSize = graphics_screen_size();

	TRY(al_script_init(&host->lua));

	lua_pushlightuserdata(host->lua, &hostKey);
	lua_pushlightuserdata(host->lua, host);
	lua_settable(host->lua, LUA_REGISTRYINDEX);

	luaL_requiref(host->lua, "host", luaopen_host, false);

	TRY(al_commands_init(host->lua));
	TRY(al_vars_init(host->lua));
	TRY(al_wrapper_init(host->lua));

	TRY(al_model_systems_init(host->lua));
	TRY(al_widget_systems_init(host, host->lua));

	TRY(al_script_run_base_scripts(host->lua));
	TRY(run_alice_scripts(host->lua));

	TRY(al_widget_init(&host->root));
	host->root->bounds = (Box2){{0, 0}, host->screenSize};
	host->keyboardWidget = host->root;

	*result = host;

	CATCH(
		al_host_free(host);
	)
	FINALLY()
}

void al_host_free(AlHost *host)
{
	if (host) {
		al_widget_free(host->root);

		al_widget_systems_free();
		al_model_systems_free();

		lua_close(host->lua);

		al_free(host);
	}
}

AlError al_host_run_script(AlHost *host, const char *filename)
{
	return al_script_run_file(host->lua, filename);
}

lua_State *al_host_get_lua(AlHost *host)
{
	return host->lua;
}

AlWidget *al_host_get_root(AlHost *host)
{
	return host->root;
}

static void handle_mouse_button(AlHost *host, SDL_MouseButtonEvent event)
{
	if (event.button != SDL_BUTTON_LEFT) {
		return;
	}

	AlWidget *widget;
	Vec2 hitLocation;

	if (host->grabbingWidget) {
		widget = host->grabbingWidget;
		hitLocation = (Vec2){0, 0};

	} else {
		Vec2 location = {event.x, host->screenSize.y - event.y};
		widget = al_widget_hit_test(host->root, location, &hitLocation);
	}

	if (widget) {
		if (event.state == SDL_PRESSED) {
			al_widget_send_down(widget, hitLocation);
		} else {
			al_widget_send_up(widget, hitLocation);
		}
	}
}

static Vec2 get_mouse_pos(AlHost *host)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	y = host->screenSize.y - y;

	return (Vec2){x, y};
}

static bool is_char(wchar_t c)
{
	return (c & 0xFF00) != 0xF700 && c > 0x001F && c != 0x007F;
}

static void handle_key_down(AlHost *host, SDL_KeyboardEvent event)
{
	al_widget_send_key(host->keyboardWidget, event.keysym.sym);
}

static void handle_text(AlHost *host, SDL_TextInputEvent event)
{
	al_widget_send_text(host->keyboardWidget, event.text);
}

void al_host_run(AlHost *host)
{
	while (!host->finished) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					handle_mouse_button(host, event.button);
					break;

				case SDL_MOUSEMOTION:
					if (host->grabbingWidget) {
						Vec2 motion = {event.motion.xrel, -event.motion.yrel};
						al_widget_send_motion(host->grabbingWidget, motion);
					}
					break;

				case SDL_KEYDOWN:
					handle_key_down(host, event.key);
					break;

				case SDL_TEXTINPUT:
					handle_text(host, event.text);
					break;

				case SDL_QUIT:
					host->finished = true;
					break;
			}
		}

		al_commands_process_queue(host->lua);

		graphics_render(host->root);

		if (!al_commands_peek_queue(host->lua)) {
			SDL_WaitEvent(NULL);
		}
	}
}

Vec2 al_host_grab_mouse(AlHost *host, AlWidget *widget)
{
	host->grabbingWidget = widget;
	Vec2 position = get_mouse_pos(host);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	return position;
}

static double clamp(double value, double min, double max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

void al_host_release_mouse(AlHost *host, Vec2 location)
{
	location.x = clamp(location.x, 0, host->screenSize.x);
	location.y = clamp(location.y, 0, host->screenSize.y);

	host->grabbingWidget = NULL;

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_WarpMouseInWindow(NULL, location.x, host->screenSize.y - location.y);
}

AlError al_host_grab_keyboard(AlHost *host, AlWidget *widget)
{
	BEGIN()

	AlWidget *oldWidget = host->keyboardWidget;
	host->keyboardWidget = widget;

	if (widget != oldWidget) {
		TRY(al_widget_send_keyboard_lost(oldWidget));
	}

	PASS()
}

AlError al_host_release_keyboard(AlHost *host)
{
	BEGIN()

	AlWidget *oldWidget = host->keyboardWidget;
	host->keyboardWidget = host->root;

	if (oldWidget != host->root) {
		TRY(al_widget_send_keyboard_lost(oldWidget));
	}

	PASS()
}

AlWidget *al_host_get_keyboard_widget(AlHost *host)
{
	return host->keyboardWidget;
}

static int cmd_exit(lua_State *L)
{
	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	host->finished = true;

	return 0;
}

static int cmd_get_root_widget(lua_State *L)
{
	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));

	al_wrapper_push_userdata(L, host->root);

	return 1;
}

static int cmd_grab_mouse(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "grab_mouse: requires 1 argument");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	AlWidget *widget = lua_touserdata(L, 1);

	Vec2 location = al_host_grab_mouse(host, widget);

	lua_pushnumber(L, location.x);
	lua_pushnumber(L, location.y);

	return 2;
}

static int cmd_release_mouse(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "release_mouse: requires 2 arguments");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	double x = lua_tonumber(L, -2);
	double y = lua_tonumber(L, -1);
	lua_pop(L, 2);

	al_host_release_mouse(host, (Vec2){x, y});

	return 0;
}

static int cmd_grab_keyboard(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "grab_keyboard: requires 1 argument");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	AlWidget *widget = lua_touserdata(L, 1);

	al_host_grab_keyboard(host, widget);

	return 0;
}

static int cmd_release_keyboard(lua_State *L)
{
	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));

	al_host_release_keyboard(host);

	return 0;
}

static const luaL_Reg lib[] = {
	{"exit", cmd_exit},
	{"get_root_widget", cmd_get_root_widget},
	{"grab_mouse", cmd_grab_mouse},
	{"release_mouse", cmd_release_mouse},
	{"grab_keyboard", cmd_grab_keyboard},
	{"release_keyboard", cmd_release_keyboard},
	{NULL, NULL}
};

static int luaopen_host(lua_State *L)
{
	luaL_newlibtable(L, lib);
	lua_pushlightuserdata(L, &hostKey);
	lua_gettable(L, LUA_REGISTRYINDEX);
	luaL_setfuncs(L, lib, 1);

	return 1;
}
