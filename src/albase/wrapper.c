/*
 * Copyright (c) 2012-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "albase/wrapper.h"
#include "albase/script.h"

static struct {
	/** Table of registered types, by type name */
	AlLuaKey types;

	/** Table of state tables for each wrapped
	 *  object to store properties against the
	 *  userdata like a regular table */
	AlLuaKey states;

	/** Table to map light userdata pointers
	 *  to full userdata */
	AlLuaKey ptrs;

	/** Table to map wrapped objects to a
	 *  retain count. Keeps objects alive while
	 *  the count is > 0 */
	AlLuaKey retained;

	/** Table to map wrapped objects to a list
	 *  of other wrapped objects that they
	 *  reference from outside Lua */
	AlLuaKey references;
} keys;

struct AlWrappedType {
	lua_State *lua;
	AlWrapperReg reg;

	AlLuaKey mt;
	AlLuaKey ctor;
};

static int luaopen_wrapper(lua_State *L);

static void build_weak_table(lua_State *L, const char *mode)
{
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushstring(L, mode);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
}

AlError al_wrapper_init(lua_State *L)
{
	BEGIN()

	lua_pushlightuserdata(L, &keys.types);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &keys.states);
	build_weak_table(L, "k");
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &keys.ptrs);
	build_weak_table(L, "v");
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &keys.retained);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &keys.references);
	build_weak_table(L, "k");
	lua_settable(L, LUA_REGISTRYINDEX);

	luaL_requiref(L, "wrapper", luaopen_wrapper, false);

	PASS()
}

static int mt_gc(lua_State *L)
{
	AlWrappedType *type = lua_touserdata(L, lua_upvalueindex(1));

	if (type->reg.free) {
		void *ptr = lua_touserdata(L, 1);
		type->reg.free(L, ptr);
	}

	return 0;
}

static int mt_index(lua_State *L)
{
	lua_pushvalue(L, 1);
	lua_gettable(L, lua_upvalueindex(1));

	lua_pushvalue(L, 2);
	lua_gettable(L, -2);

	return 1;
}

static int mt_newindex(lua_State *L)
{
	lua_pushvalue(L, 1);
	lua_gettable(L, lua_upvalueindex(1));

	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_settable(L, -3);

	return 0;
}

static void build_mt(lua_State *L, AlWrappedType *type)
{
	lua_newtable(L);

	lua_pushliteral(L, "__gc");
	lua_pushlightuserdata(L, type);
	lua_pushcclosure(L, mt_gc, 1);
	lua_settable(L, -3);

	lua_pushliteral(L, "__index");
	lua_pushlightuserdata(L, &keys.states);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushcclosure(L, mt_index, 1);
	lua_settable(L, -3);

	lua_pushliteral(L, "__newindex");
	lua_pushlightuserdata(L, &keys.states);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushcclosure(L, mt_newindex, 1);
	lua_settable(L, -3);
}

static int base_ctor(lua_State *L)
{
	BEGIN()

	AlWrappedType *type = lua_touserdata(L, lua_upvalueindex(1));

	void *ptr = lua_newuserdata(L, type->reg.size);

	lua_pushlightuserdata(L, &keys.states);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -2);
	lua_newtable(L);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushlightuserdata(L, &type->mt);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_setmetatable(L, -2);

	lua_pushlightuserdata(L, &keys.ptrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 1);

	TRY(type->reg.init(L, ptr, type->reg.initData));

	CATCH_LUA(, "Error constructing %s", type->reg.name)
	FINALLY_LUA(, 1)
}

static void init_type_tables(lua_State *L, AlWrappedType *type)
{
	lua_pushlightuserdata(L, &type->mt);
	build_mt(L, type);
	lua_settable(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, &type->ctor);
	lua_pushlightuserdata(L, type);
	lua_pushcclosure(L, base_ctor, 1);
	lua_settable(L, LUA_REGISTRYINDEX);
}

static void register_type(lua_State *L, AlWrappedType *type)
{
	lua_pushlightuserdata(L, &keys.types);
	lua_gettable(L, LUA_REGISTRYINDEX);

	lua_pushstring(L, type->reg.name);
	lua_pushlightuserdata(L, type);
	lua_settable(L, -3);

	lua_pop(L, 1);
}

AlError al_wrapper_register(lua_State *L, AlWrapperReg reg, AlWrappedType **result)
{
	BEGIN()

	AlWrappedType *type = NULL;
	TRY(al_malloc(&type, sizeof(AlWrappedType)));

	type->lua = L;
	type->reg = reg;

	init_type_tables(L, type);
	register_type(L, type);

	*result = type;

	PASS()
}

AlError al_wrapper_invoke_ctor(AlWrappedType *type, void *result)
{
	BEGIN()

	lua_State *L = type->lua;

	lua_pushlightuserdata(L, &type->ctor);
	lua_gettable(L, LUA_REGISTRYINDEX);
	TRY(al_script_call(L, 0, 1));

	*(void **)result = lua_touserdata(L, -1);
	al_wrapper_retain(L, *(void **)result);

	lua_pop(L, 1);

	PASS()
}

void al_wrapper_retain(lua_State *L, void *ptr)
{
	lua_pushlightuserdata(L, &keys.retained);
	lua_gettable(L, LUA_REGISTRYINDEX);
	al_wrapper_push_userdata(L, ptr);
	lua_gettable(L, -2);

	lua_Number numRefs;

	if (lua_isnil(L, -1)) {
		numRefs = 0;
	} else {
		numRefs = lua_tointeger(L, -1);
	}

	lua_pop(L, 1);

	al_wrapper_push_userdata(L, ptr);
	lua_pushinteger(L, numRefs + 1);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

void al_wrapper_release(lua_State *L, void *ptr)
{
	if (!ptr)
		return;

	lua_pushlightuserdata(L, &keys.retained);
	lua_gettable(L, LUA_REGISTRYINDEX);
	al_wrapper_push_userdata(L, ptr);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		return;
	}

	lua_Number numRefs = lua_tointeger(L, -1);
	lua_pop(L, 1);

	al_wrapper_push_userdata(L, ptr);

	if (numRefs > 1) {
		lua_pushinteger(L, numRefs - 1);
	} else {
		lua_pushnil(L);
	}

	lua_settable(L, -3);
	lua_pop(L, 1);
}

void al_wrapper_push_userdata(lua_State *L, void *ptr)
{
	lua_pushlightuserdata(L, &keys.ptrs);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);
	lua_replace(L, -2);
}

void al_wrapper_reference(lua_State *L)
{
	lua_pushlightuserdata(L, &keys.references);
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

void al_wrapper_unreference(lua_State *L)
{
	lua_pushlightuserdata(L, &keys.references);
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

static AlWrappedType *get_type(lua_State *L, const char *name)
{
	lua_pushlightuserdata(L, &keys.types);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	AlWrappedType *type = lua_touserdata(L, -1);
	lua_pop(L, 2);

	if (!type)
		luaL_error(L, "no such type: '%s'", name);

	return type;
}

static int cmd_wrap_ctor(lua_State *L)
{
	const char *typeName = luaL_checkstring(L, 1);
	AlWrappedType *type = get_type(L, typeName);

	lua_pushvalue(L, 2);
	lua_pushlightuserdata(L, &type->ctor);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_call(L, 1, 1);

	lua_pushlightuserdata(L, &type->ctor);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	return 1;
}

static int cmd_set_prototype(lua_State *L)
{
	lua_pushlightuserdata(L, &keys.states);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	lua_gettable(L, -2);

	lua_newtable(L);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, 2);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);

	lua_pop(L, 3);

	return 1;
}

static const luaL_Reg lib[] = {
	{"wrap_ctor", cmd_wrap_ctor},
	{"set_prototype", cmd_set_prototype},
	{NULL, NULL}
};

static int luaopen_wrapper(lua_State *L)
{
	luaL_newlib(L, lib);

	return 1;
}
