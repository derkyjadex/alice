/*
 * Copyright (c) 2011-2014 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "albase/commands.h"
#include "albase/lua.h"
#include "albase/script.h"

typedef struct {
	lua_Number first;
	lua_Number last;
} QueueInfo;

static struct {
	AlLuaKey queue;
	AlLuaKey queueInfo;
} keys;

static int luaopen_commands(lua_State *L);

AlError al_commands_init(lua_State *L)
{
	BEGIN()

	lua_pushlightuserdata(L, &keys.queue);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &keys.queueInfo);
	lua_newuserdata(L, sizeof(QueueInfo));
	QueueInfo *info = lua_touserdata(L, -1);
	lua_settable(L, LUA_REGISTRYINDEX);

	info->first = 0;
	info->last = -1;

	luaL_requiref(L, "commands", luaopen_commands, false);

	PASS()
}

static QueueInfo *get_info(lua_State *L)
{
	lua_pushlightuserdata(L, &keys.queueInfo);
	lua_gettable(L, LUA_REGISTRYINDEX);
	QueueInfo *info = lua_touserdata(L, -1);
	lua_pop(L, 1);

	return info;
}

AlError al_commands_enqueue(lua_State *L)
{
	BEGIN()

	QueueInfo *info = get_info(L);
	info->last++;

	lua_pushlightuserdata(L, &keys.queue);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushnumber(L, info->last);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 2);

	PASS()
}

AlError al_commands_process_queue(lua_State *L)
{
	QueueInfo *info = get_info(L);
	lua_Number last = info->last;

	al_script_push_traceback(L);

	lua_pushlightuserdata(L, &keys.queue);
	lua_gettable(L, LUA_REGISTRYINDEX);

	while (info->first <= last) {
		lua_pushnumber(L, info->first);
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

		lua_pushnumber(L, info->first);
		lua_pushnil(L);
		lua_settable(L, -3);

		info->first++;
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

	TRY(al_commands_enqueue(L));
	lua_pop(L, n);

	CATCH_LUA(, "Error queueing command")
	FINALLY_LUA(, 0)
}

static const luaL_Reg lib[] = {
	{"enqueue", cmd_enqueue},
	{NULL, NULL}
};

static int luaopen_commands(lua_State *L)
{
	luaL_newlib(L, lib);

	return 1;
}
