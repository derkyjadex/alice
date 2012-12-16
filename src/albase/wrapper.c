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
	size_t objSize;
	AlWrapperFree free;

	AlLuaKey ctor;
	AlLuaKey ptrs;
	AlLuaKey retained;
	AlLuaKey references;
};

AlError al_wrapper_init(AlWrapper **result, lua_State *L, size_t objSize, AlWrapperFree free)
{
	BEGIN()

	AlWrapper *wrapper = NULL;
	TRY(al_malloc(&wrapper, sizeof(AlWrapper), 1));

	wrapper->lua = L;
	wrapper->objSize = objSize;
	wrapper->free = free;

	lua_pushlightuserdata(L, &wrapper->ptrs);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &wrapper->retained);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &wrapper->references);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "k");
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	*result = wrapper;

	PASS()
}

void al_wrapper_free(AlWrapper *wrapper)
{
	if (wrapper) {
		lua_State *L = wrapper->lua;

		lua_pushlightuserdata(L, &wrapper->ctor);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(L, &wrapper->ptrs);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(L, &wrapper->retained);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(L, &wrapper->references);
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

static int cmd_register_ctor(lua_State *L)
{
	BEGIN()

	AlWrapper *wrapper = lua_touserdata(L, lua_upvalueindex(1));

	luaL_checkany(L, 1);
	TRY(al_wrapper_register_ctor(wrapper));

	CATCH(
		return luaL_error(L, "Error registering constructor");
	)
	FINALLY(
		return 0;
	)
}

AlError al_wrapper_register_register_ctor_command(AlWrapper *wrapper, const char *name, AlCommands *commands)
{
	return al_commands_register(commands, name, cmd_register_ctor, wrapper, NULL);
}

AlError al_wrapper_invoke_ctor(AlWrapper *wrapper, void *result)
{
	BEGIN()

	lua_State *L =wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ctor);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_call(L, 0, 1);

	*(void **)result = lua_touserdata(L, -1);
	lua_pop(L, 1);

	PASS()
}

static int gc(lua_State *L)
{
	AlWrapper *wrapper = lua_touserdata(L, lua_upvalueindex(1));
	void *ptr = lua_touserdata(L, 1);

	wrapper->free(L, ptr);

	return 0;
}

AlError al_wrapper_create(AlWrapper *wrapper, void *result)
{
	BEGIN()

	lua_State *L = wrapper->lua;

	void *obj = lua_newuserdata(L, wrapper->objSize);

	lua_newtable(L);
	lua_pushliteral(L, "__gc");
	lua_pushlightuserdata(L, wrapper);
	lua_pushcclosure(L, gc, 1);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	lua_pushlightuserdata(L, &wrapper->ptrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, obj);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 2);

	*(void **)result = obj;

	PASS()
}

void al_wrapper_retain(AlWrapper *wrapper, void *obj)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->retained);
	lua_gettable(L, LUA_REGISTRYINDEX);
	al_wrapper_push_userdata(wrapper, obj);
	lua_gettable(L, -2);

	lua_Number numRefs;

	if (lua_isnil(L, -1)) {
		numRefs = 0;
	} else {
		numRefs = lua_tointeger(L, -1);
	}

	lua_pop(L, 1);

	al_wrapper_push_userdata(wrapper, obj);
	lua_pushinteger(L, numRefs + 1);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

void al_wrapper_release(AlWrapper *wrapper, void *obj)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->retained);
	lua_gettable(L, LUA_REGISTRYINDEX);
	al_wrapper_push_userdata(wrapper, obj);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		return;
	}

	lua_Number numRefs = lua_tointeger(L, -1);
	lua_pop(L, 1);

	al_wrapper_push_userdata(wrapper, obj);

	if (numRefs > 1) {
		lua_pushinteger(L, numRefs - 1);
	} else {
		lua_pushnil(L);
	}

	lua_settable(L, -3);
	lua_pop(L, 1);
}

void al_wrapper_push_userdata(AlWrapper *wrapper, void *obj)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->ptrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, -2);
	lua_replace(L, -2);
}

void al_wrapper_reference(AlWrapper *wrapper)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->references);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -3);
	lua_gettable(L, -2);

	lua_Number numRefs;

	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -4);
		lua_pushvalue(L, -2);
		lua_settable(L, -4);

		numRefs = 0;

	} else {
		lua_pushvalue(L, -3);
		lua_gettable(L, -2);

		numRefs = lua_tointeger(L, -1);

		lua_pop(L, 1);
	}

	lua_pushvalue(L, -3);
	lua_pushinteger(L, numRefs + 1);
	lua_settable(L, -3);

	lua_pop(L, 4);
}

void al_wrapper_unreference(AlWrapper *wrapper)
{
	lua_State *L = wrapper->lua;

	lua_pushlightuserdata(L, &wrapper->references);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -3);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 4);
		return;
	}

	lua_pushvalue(L, -3);
	lua_gettable(L, -2);
	lua_Number numRefs = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushvalue(L, -3);

	if (numRefs > 1) {
		lua_pushinteger(L, numRefs - 1);
	} else {
		lua_pushnil(L);
	}

	lua_settable(L, -3);

	lua_pop(L, 4);
}
