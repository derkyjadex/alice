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
	AlLuaKey gcListeners;
	AlWrapperFree free;
};

static void make_weak(lua_State *L, const char *mode)
{
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushstring(L, mode);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
}

AlError al_wrapper_init(AlWrapper **result, lua_State *L, bool weak, AlWrapperFree free)
{
	BEGIN()

	AlWrapper *wrapper = NULL;
	TRY(al_malloc(&wrapper, sizeof(AlWrapper), 1));

	wrapper->lua = L;
	wrapper->ctor = false;
	wrapper->free = free;

	lua_pushlightuserdata(L, &wrapper->tablePtrs);
	lua_newtable(L);
	if (weak) {
		make_weak(L, "k");
	}
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_newtable(L);
	if (weak) {
		make_weak(L, "v");
	}
	lua_settable(L, LUA_REGISTRYINDEX);

	if (free) {
		lua_pushlightuserdata(L, &wrapper->gcListeners);
		lua_newtable(L);
		make_weak(L, "k");
		lua_settable(L, LUA_REGISTRYINDEX);
	}

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

		lua_pushlightuserdata(L, &wrapper->gcListeners);
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

static int free_ptr(lua_State *L)
{
	AlWrapper *wrapper = lua_touserdata(L, lua_upvalueindex(1));
	void *ptr = lua_touserdata(L, lua_upvalueindex(2));

	wrapper->free(L, ptr);

	return 0;
}

static void auto_free(AlWrapper *wrapper, void *ptr)
{
	lua_State *L = wrapper->lua;

	lua_newuserdata(L, 0);
	lua_newtable(L);
	lua_pushliteral(L, "__gc");
	lua_pushlightuserdata(L, wrapper);
	lua_pushlightuserdata(L, ptr);
	lua_pushcclosure(L, free_ptr, 2);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	lua_pushlightuserdata(L, &wrapper->gcListeners);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -3);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 2);
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
	lua_pop(L, 1);

	if (wrapper->free) {
		auto_free(wrapper, ptr);
	}
	lua_pop(L, 1);

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

	if (wrapper->free) {
		lua_pushlightuserdata(L, &wrapper->gcListeners);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, -2);
		lua_gettable(L, -2);
		lua_pushnil(L);
		lua_setmetatable(L, -2);
		lua_pop(L, 2);
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

void al_wrapper_wrap(AlWrapper *wrapper, void *ptr, int nArgs)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ptrTables);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);
	lua_remove(L, -2);

	if (lua_isnil(L, -1)) {
		if (!wrapper->ctor) {
			luaL_error(L, "Cannot wrap pointer, wrapper has no constructor set");
			return;
		}

		lua_pop(L, 1);
		lua_pushlightuserdata(L, &wrapper->ctor);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(L, ptr);
		for (int i = 0; i < nArgs; i++) {
			lua_pushvalue(L, -2 - nArgs);
			lua_remove(L, -3 - nArgs);
		}
		lua_call(L, 1 + nArgs, 1);
	}
}
