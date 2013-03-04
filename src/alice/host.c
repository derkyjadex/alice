/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <SDL/SDL.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>

#include "alice/host.h"
#include "albase/geometry.h"
#include "graphics.h"
#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"
#include "file_system.h"
#include "text.h"
#include "widget_internal.h"

struct AlHost {
	lua_State *lua;
	AlCommands *commands;
	AlVars *vars;
	bool finished;

	Vec2 screenSize;

	AlWidget *root;
	AlWidget *grabbingWidget;
	AlWidget *keyboardWidget;
};

static const int IGNORE_NEXT_MOTION_EVENT = 1;
static int cmd_exit(lua_State *L);
static int cmd_get_root_widget(lua_State *L);
static int cmd_grab_mouse(lua_State *L);
static int cmd_release_mouse(lua_State *L);
static int cmd_grab_keyboard(lua_State *L);
static int cmd_release_keyboard(lua_State *L);

#ifdef RASPI
static bool showMouse = true;
#endif

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

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);

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

AlError al_host_init(AlHost **result)
{
	BEGIN()

	AlHost *host = NULL;
	TRY(al_malloc(&host, sizeof(AlHost), 1));

	host->lua = NULL;
	host->commands = NULL;
	host->vars = NULL;
	host->finished = false;
	host->screenSize = (Vec2){1, 1};
	host->root = NULL;
	host->grabbingWidget = NULL;
	host->keyboardWidget = NULL;

	host->screenSize = graphics_screen_size();

	TRY(al_script_init(&host->lua));
	TRY(al_commands_init(&host->commands, host->lua));
	TRY(al_vars_init(&host->vars, host->lua, host->commands));

	TRY(al_commands_register(host->commands, "exit", cmd_exit, host, NULL));
	TRY(al_commands_register(host->commands, "get_root_widget", cmd_get_root_widget, host, NULL));
	TRY(al_commands_register(host->commands, "grab_mouse", cmd_grab_mouse, host, NULL));
	TRY(al_commands_register(host->commands, "release_mouse", cmd_release_mouse, host, NULL));
	TRY(al_commands_register(host->commands, "grab_keyboard", cmd_grab_keyboard, host, NULL));
	TRY(al_commands_register(host->commands, "release_keyboard", cmd_release_keyboard, host, NULL));

	TRY(al_model_systems_init(host->lua, host->commands, host->vars));
	TRY(al_widget_systems_init(host->lua, host->commands, host->vars));
	TRY(file_system_init(host->commands));
	TRY(text_system_init(host->commands));

	AlScript scripts[] = {
		AL_SCRIPT(widget),
		AL_SCRIPT(draggable),
		AL_SCRIPT(toolbar),
		AL_SCRIPT(slider_widget),
		AL_SCRIPT(colour_widget),
		AL_SCRIPT(model_widget),
		AL_SCRIPT(file_widget),
		AL_SCRIPT(text_box),
		AL_SCRIPT(panning_widget),
		AL_SCRIPT(model_view_model),
		AL_SCRIPT_END
	};

	TRY(al_script_run_base_scripts(host->lua));
	TRY(al_script_run_scripts(host->lua, scripts));

	TRY(al_widget_init(&host->root));
	host->root->bounds = (Box){{0, 0}, host->screenSize};
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
		al_commands_free(host->commands);
		al_vars_free(host->vars);
		lua_close(host->lua);
		free(host);

		al_model_systems_free();
		al_widget_systems_free();
	}
}

AlError al_host_run_script(AlHost *host, const char *filename)
{
	return al_script_run_file(host->lua, filename);
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
	static char text[MB_LEN_MAX + 1] = "";

	if (is_char(event.keysym.unicode)) {
		int numBytes = wctomb(text, event.keysym.unicode);
		text[numBytes] = '\0';

		al_widget_send_text(host->keyboardWidget, text);

	} else {
		al_widget_send_key(host->keyboardWidget, event.keysym.sym);
	}
}

void al_host_run(AlHost *host)
{
	SDL_Event event;
	bool ignoreNextMotion = false;

	while (!host->finished) {

		SDL_WaitEvent(&event);
		do {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					handle_mouse_button(host, event.button);
					break;

				case SDL_MOUSEMOTION:
#ifdef RASPI
					widget_invalidate(host->root);
#endif
					if (ignoreNextMotion) {
						ignoreNextMotion = false;

					} else if (host->grabbingWidget) {
						Vec2 motion = {event.motion.xrel, -event.motion.yrel};
						al_widget_send_motion(host->grabbingWidget, motion);
					}
					break;

				case SDL_KEYDOWN:
					handle_key_down(host, event.key);
					break;

				case SDL_USEREVENT:
					if (event.user.code == IGNORE_NEXT_MOTION_EVENT)
						ignoreNextMotion = true;

					break;

				case SDL_QUIT:
					host->finished = true;
					break;
			}
		} while (SDL_PollEvent(&event));

		al_commands_process_queue(host->commands);

#ifdef RASPI
		graphics_render(host->root, showMouse, get_mouse_pos(host));
#else
		graphics_render(host->root, false, (Vec2){0, 0});
#endif
	}
}

static Vec2 host_grab_mouse(AlHost *host, AlWidget *widget)
{
	host->grabbingWidget = widget;

	SDL_Event ignoreEvent;
	ignoreEvent.type = SDL_USEREVENT;
	ignoreEvent.user.code = IGNORE_NEXT_MOTION_EVENT;
	SDL_PushEvent(&ignoreEvent);

	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);

#ifdef RASPI
	showMouse = false;
#endif

	return get_mouse_pos(host);
}

static double clamp(double value, double min, double max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

static void host_release_mouse(AlHost *host, Vec2 location)
{
	location.x = clamp(location.x, 0, host->screenSize.x);
	location.y = clamp(location.y, 0, host->screenSize.y);

	host->grabbingWidget = NULL;
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(SDL_ENABLE);
	SDL_WarpMouse(location.x, host->screenSize.y - location.y);

#ifdef RASPI
	showMouse = true;
#endif
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

	al_widget_push_userdata(host->root);

	return 1;
}

static int cmd_grab_mouse(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "grab_mouse: requires 1 argument");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	AlWidget *widget = lua_touserdata(L, 1);

	Vec2 location = host_grab_mouse(host, widget);

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

	host_release_mouse(host, (Vec2){x, y});

	return 0;
}

static int cmd_grab_keyboard(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "grab_keyboard: requires 1 argument");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	AlWidget *widget = lua_touserdata(L, 1);

	AlWidget *oldWidget = host->keyboardWidget;
	host->keyboardWidget = widget;

	if (widget != oldWidget) {
		al_widget_send_keyboard_lost(oldWidget);
	}

	return 0;
}

static int cmd_release_keyboard(lua_State *L)
{
	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));

	AlWidget *oldWidget = host->keyboardWidget;
	host->keyboardWidget = host->root;

	if (oldWidget != host->root) {
		al_widget_send_keyboard_lost(oldWidget);
	}

	return 0;
}
