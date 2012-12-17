/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"

static AlLuaKey tracebackKey;

static int traceback(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 1;

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);

	return 1;
}

AlError al_script_init(lua_State **result)
{
	BEGIN()

	lua_State *L = NULL;

	L = luaL_newstate();
	if (!L)
		THROW(AL_ERROR_MEMORY)

	luaL_openlibs(L);

	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushcclosure(L, traceback, 1);
	lua_pushlightuserdata(L, &tracebackKey);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 2);

	*result = L;

	CATCH(
		lua_close(L);
	)
	FINALLY()
}

static AlError run_script(lua_State *L, int loadResult)
{
	BEGIN()

	if (!loadResult) {
		al_script_push_traceback(L);
		lua_pushvalue(L, -2);

		loadResult = lua_pcall(L, 0, 0, -2);
	}

	if (loadResult) {
		const char *message = lua_tostring(L, -1);
		al_log_error("Error running script: \n%s", message);
		lua_pop(L, 1);
		THROW(AL_ERROR_SCRIPT)
	}

	PASS(
		lua_pop(L, 2);
	)
}

AlError al_script_run_base_scripts(lua_State *L)
{
	BEGIN()

	AlScript scripts[] = {
		AL_SCRIPT(common),
		AL_SCRIPT(class),
		AL_SCRIPT(wrapper),
		AL_SCRIPT(model),
		AL_SCRIPT_END
	};

	TRY(al_script_run_scripts(L, scripts));

	PASS()
}

AlError al_script_run_scripts(lua_State *L, const AlScript *scripts)
{
	BEGIN()

	int result;
	for (const AlScript *script = scripts; script->source; script++) {
		result = luaL_loadbuffer(L, script->source, script->length, script->name);
		TRY(run_script(L, result));
	}

	PASS()
}

AlError al_script_run_file(lua_State *L, const char *filename)
{
	BEGIN()

	int result = luaL_loadfile(L, filename);
	TRY(run_script(L, result));

	PASS()
}

void al_script_push_traceback(lua_State *L)
{
	lua_pushlightuserdata(L, &tracebackKey);
	lua_gettable(L, LUA_REGISTRYINDEX);
}
