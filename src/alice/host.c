/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <SDL/SDL.h>
#include <locale.h>

#include "alice/host.h"
#include "albase/geometry.h"
#include "widget_graphics.h"
#include "model_editing.h"
#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"
#include "file_system.h"

static const int IGNORE_NEXT_MOTION_EVENT = 1;
static int cmd_exit(lua_State *L);
static int cmd_get_root_widget(lua_State *L);
static int cmd_grab_mouse(lua_State *L);
static int cmd_release_mouse(lua_State *L);

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

	TRY(widget_graphics_system_init());

	const char *locale = "UTF-8";
	char *result = setlocale(LC_CTYPE, locale);
	if (strcmp(locale, result)) {
		al_log_error("Could not set locale to %s", locale);
		THROW(AL_ERROR_GENERIC);
	}

	CATCH(
		al_host_systems_free();
	)
	FINALLY()
}

void al_host_systems_free()
{
	widget_graphics_system_free();
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
	host->widgets = NULL;
	host->grabbingWidget = NULL;

	host->screenSize = widget_graphics_screen_size();

	TRY(al_script_init(&host->lua));
	TRY(al_commands_init(&host->commands, host->lua));
	TRY(al_vars_init(&host->vars, host->lua, host->commands));
	TRY(widget_init(&host->widgets, host->lua, host->commands));
	host->widgets->bounds = (Box){{0, 0}, host->screenSize};

	TRY(al_commands_register(host->commands, "exit", cmd_exit, host, NULL));
	TRY(al_commands_register(host->commands, "get_root_widget", cmd_get_root_widget, host, NULL));
	TRY(al_commands_register(host->commands, "grab_mouse", cmd_grab_mouse, host, NULL));
	TRY(al_commands_register(host->commands, "release_mouse", cmd_release_mouse, host, NULL));
	TRY(widget_register_commands(host->commands));
	TRY(model_editing_register_commands(host->commands));
	TRY(file_system_register_commands(host->commands));

	TRY(widget_register_vars(host->vars));

	AlScript scripts[] = {
		AL_SCRIPT(widget),
		AL_SCRIPT(model),
		AL_SCRIPT(draggable),
		AL_SCRIPT(toolbar),
		AL_SCRIPT(slider_widget),
		AL_SCRIPT(colour_widget),
		AL_SCRIPT(model_widget),
		AL_SCRIPT(file_widget),
		AL_SCRIPT_END
	};

	TRY(al_script_run_base_scripts(host->lua));
	TRY(al_script_run_scripts(host->lua, scripts));

	*result = host;

	CATCH(
		al_host_free(host);
	)
	FINALLY()
}

void al_host_free(AlHost *host)
{
	if (host) {
		al_commands_free(host->commands);
		al_vars_free(host->vars);
		widget_free(host->widgets);
		lua_close(host->lua);
		free(host);
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

	if (host->grabbingWidget) {
		widget_send_up(host->grabbingWidget);
		return;
	}

	Vec2 location = {event.x, host->screenSize.y - event.y};
	AlWidget *hit = widget_hit_test(host->widgets, location);

	if (hit) {
		if (event.state == SDL_PRESSED)
			widget_send_down(hit);
		else
			widget_send_up(hit);
	}
}

static Vec2 get_mouse_pos(AlHost *host)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	y = host->screenSize.y - y;

	return (Vec2){x, y};
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
					if (ignoreNextMotion) {
						ignoreNextMotion = false;

					} else if (host->grabbingWidget) {
						Vec2 motion = {event.motion.xrel, -event.motion.yrel};
						widget_send_motion(host->grabbingWidget, motion);
					}
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
		widget_graphics_render(host->widgets, showMouse, get_mouse_pos(host));
#else
		widget_graphics_render(host->widgets, false, (Vec2){0, 0});
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

	lua_pushlightuserdata(L, host->widgets);

	return 1;
}

static int cmd_grab_mouse(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "grab_mouse: requires 1 argument");

	AlHost *host = lua_touserdata(L, lua_upvalueindex(1));
	AlWidget *widget = lua_touserdata(L, -1);
	lua_pop(L, 1);

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
