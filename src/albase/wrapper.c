/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stdbool.h>

#include "albase/wrapper.h"

struct AlWrapper {
	lua_State *lua;
	AlLuaKey tablePtrs;
	AlLuaKey ptrTables;
	AlLuaKey ctor;
};

AlError al_wrapper_init(AlWrapper **result, lua_State *L)
{
	BEGIN()

	AlWrapper *wrapper = NULL;
	TRY(al_malloc(&wrapper, sizeof(AlWrapper), 1));

	wrapper->lua = L;
	wrapper->ctor = false;

	lua_pushlightuserdata(L, &wrapper->tablePtrs);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	*result = wrapper;

	CATCH(
		al_wrapper_free(wrapper);
	)
	FINALLY()
}

void al_wrapper_free(AlWrapper *wrapper)
{
	if (wrapper) {
		lua_State *L = wrapper->lua;

		lua_pushlightuserdata(L, &wrapper->tablePtrs);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(L, &wrapper->ptrTables);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		free(wrapper);
	}
}

AlError al_wrapper_register_ctor(AlWrapper *wrapper)
{
	BEGIN()

	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ctor);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	wrapper->ctor = true;

	PASS()
}

AlError al_wrapper_register(AlWrapper *wrapper, void *ptr, int tableN)
{
	BEGIN()

	lua_State *L = wrapper->lua;

	lua_pushvalue(L, tableN);

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushlightuserdata(L, &wrapper->tablePtrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -2);
	lua_pushlightuserdata(L, ptr);
	lua_settable(L, -3);
	lua_pop(L, 2);

	PASS()
}

void al_wrapper_unregister(AlWrapper *wrapper, void *ptr)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, &wrapper->tablePtrs);
	lua_gettable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -3);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 3);
		return;
	}

	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushlightuserdata(L, ptr);
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

void *al_wrapper_unwrap(AlWrapper *wrapper)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->tablePtrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -2);
	lua_gettable(L, -2);

	void *ptr = NULL;

	if (!lua_isnil(L, -1)) {
		ptr = lua_touserdata(L, -1);
	}

	lua_pop(L, 3);

	return ptr;
}

void al_wrapper_wrap(AlWrapper *wrapper, void *ptr)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);
	lua_remove(L, -2);

	if (lua_isnil(L, -1)) {
		if (!wrapper->ctor) {
			luaL_error(L, "Cannot wrap pointer, wrapper has not constructor set");
			return;
		}

		lua_pop(L, 1);
		lua_pushlightuserdata(L, &wrapper->ctor);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(L, ptr);
		lua_call(L, 1, 1);
	}
}
