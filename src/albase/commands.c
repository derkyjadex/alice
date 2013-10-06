/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "albase/commands.h"
#include "albase/lua.h"
#include "albase/script.h"

struct AlCommands {
	lua_State *lua;
	AlLuaKey queue;
	lua_Number first;
	lua_Number last;
};

static int cmd_enqueue(lua_State *L);

AlError al_commands_init(AlCommands **result, lua_State *lua)
{
	BEGIN()

	AlCommands *commands = NULL;
	TRY(al_malloc(&commands, sizeof(AlCommands)));

	commands->lua = lua;
	commands->first = 0;
	commands->last = -1;

	lua_newtable(lua);
	lua_setglobal(lua, "commands");

	lua_pushlightuserdata(lua, &commands->queue);
	lua_newtable(lua);
	lua_settable(lua, LUA_REGISTRYINDEX);

	TRY(al_commands_register(commands, "enqueue", &cmd_enqueue, commands, NULL));

	*result = commands;

	PASS()
}

void al_commands_free(AlCommands *commands)
{
	if (commands != NULL) {
		lua_State *L = commands->lua;
		lua_pushnil(L);
		lua_setglobal(L, "commands");
		lua_pushlightuserdata(L, &commands->queue);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		free(commands);
	}
}

AlError al_commands_register(AlCommands *commands, const char *name, lua_CFunction function, ...)
{
	lua_State *L = commands->lua;
	lua_getglobal(L, "commands");
	lua_pushstring(L, name);

	void *data;
	int n = 0;
	va_list ap;
	va_start(ap, function);
	while ((data = va_arg(ap, void *))) {
		lua_pushlightuserdata(L, data);
		n++;
	}
	va_end(ap);

	lua_pushcclosure(L, function, n);
	lua_settable(L, -3);
	lua_pop(L, 1);

	return AL_NO_ERROR;
}

AlError al_commands_enqueue(AlCommands *commands)
{
	commands->last++;

	lua_State *L = commands->lua;
	lua_pushlightuserdata(L, &commands->queue);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushnumber(L, commands->last);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 2);

	return AL_NO_ERROR;
}

AlError al_commands_process_queue(AlCommands *commands)
{
	lua_State *L = commands->lua;
	al_script_push_traceback(L);

	lua_pushlightuserdata(L, &commands->queue);
	lua_gettable(L, LUA_REGISTRYINDEX);

	while (commands->first <= commands->last) {
		lua_pushnumber(L, commands->first);
		lua_gettable(L, -2);

		int n = (int)lua_rawlen(L, -1);
		int i;
		for (i = 1; i <= n; i++) {
			lua_pushnumber(L, i);
			lua_gettable(L, -1 - i);
		}
		int result = lua_pcall(L, n - 1, 0, -n - 3);
		if (result != 0) {
			const char *message = lua_tostring(L, -1);
			al_log_error("Error executing command: \n%s", message);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		lua_pushnumber(L, commands->first);
		lua_pushnil(L);
		lua_settable(L, -3);

		commands->first++;
	}

	lua_pop(L, 2);

	return AL_NO_ERROR;
}

static int cmd_enqueue(lua_State *L)
{
	BEGIN()

	int n = lua_gettop(L);
	if (n < 1) {
		return luaL_error(L, "enqueue: enqueue requires at least one argument");
	}

	lua_newtable(L);
	for (int i = 1; i <= n; i++) {
		lua_pushinteger(L, i);
		lua_pushvalue(L, i - n - 3);
		lua_settable(L, -3);
	}

	AlCommands *commands = lua_touserdata(L, lua_upvalueindex(1));
	TRY(al_commands_enqueue(commands));
	lua_pop(L, n);

	CATCH_LUA(, "Error queueing command")
	FINALLY_LUA(, 0)
}
