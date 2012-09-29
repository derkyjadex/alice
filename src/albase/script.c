/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"

AlError al_init_lua(lua_State **result)
{
	BEGIN()

	lua_State *L = NULL;

	L = luaL_newstate();
	if (!L)
		THROW(AL_ERROR_MEMORY)

		luaL_Reg luaLibs[] = {
			{"", luaopen_base},
			{LUA_MATHLIBNAME, luaopen_math},
			{LUA_LOADLIBNAME, luaopen_package},
			{LUA_TABLIBNAME, luaopen_table},
			{NULL, NULL}
		};

	for (luaL_Reg *lib = luaLibs; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}

	*result = L;

	CATCH(
		lua_close(L);
	)
	FINALLY()
}

static AlError run_script(lua_State *L, const char *script)
{
	BEGIN()

	int result = luaL_dostring(L, script);
	if (result) {
		const char *message = lua_tostring(L, -1);
		al_log_error("Error running script: %s", message);
		lua_pop(L, 1);
		THROW(AL_ERROR_SCRIPT)
	}

	PASS()
}

AlError al_load_base_scripts(lua_State *L)
{
	BEGIN()

	const char *scripts[] = {
		scripts_common_lua,
		scripts_class_lua,
		scripts_wrapper_lua,
		NULL
	};

	TRY(al_load_scripts(L, scripts));

	PASS()
}

AlError al_load_scripts(lua_State *L, const char **scripts)
{
	BEGIN()

	FOR_EACH(const char, script, scripts) {
		TRY(run_script(L, script));
	}

	PASS()
}
