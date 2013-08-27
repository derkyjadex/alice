/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <string.h>
#include <stdlib.h>

#include "albase/vars.h"
#include "albase/geometry.h"
#include "albase/lua.h"

struct AlVars {
	lua_State *lua;
	AlLuaKey entries;
};

typedef char * String;

static AlLuaKey varsKey;
static int luaopen_vars(lua_State *L);

AlError al_vars_init(AlVars **result, lua_State *lua)
{
	BEGIN()

	AlVars *vars = NULL;
	TRY(al_malloc(&vars, sizeof(AlVars), 1));

	vars->lua = lua;

	lua_pushlightuserdata(lua, &vars->entries);
	lua_newtable(lua);
	lua_settable(lua, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(lua, &varsKey);
	lua_pushlightuserdata(lua, vars);
	lua_settable(lua, LUA_REGISTRYINDEX);

	luaL_requiref(lua, "vars", luaopen_vars, false);

	*result = vars;

	CATCH(
		free(vars);
	)
	FINALLY()
}

void al_vars_free(AlVars *vars)
{
	if (vars != NULL) {
		lua_State *L = vars->lua;
		lua_pushlightuserdata(L, &vars->entries);
		lua_gettable(L, LUA_REGISTRYINDEX);

		lua_pushnil(L);
		while (lua_next(L, -2)) {
			AlVarReg *entry = lua_touserdata(L, -1);
			free(entry);
			lua_pop(L, 1);
		}

		lua_pop(L, 1);

		lua_pushlightuserdata(L, &vars->entries);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		free(vars);
	}
}

AlError al_vars_register(AlVars *vars, AlVarReg reg)
{
	BEGIN()

	AlVarReg *entry = NULL;
	TRY(al_malloc(&entry, sizeof(AlVarReg), 1));

	*entry = reg;
	entry->name = NULL;

	lua_State *L = vars->lua;
	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, reg.name);
	lua_pushlightuserdata(L, entry);
	lua_settable(L, -3);

	PASS()
}

static bool tobool(lua_State *L, int n)
{
	luaL_checkany(L, n);
	return lua_toboolean(L, n);
}

static Vec2 toVec2(lua_State *L, int n)
{
	double x = luaL_checknumber(L, n + 0);
	double y = luaL_checknumber(L, n + 1);

	return (Vec2){x, y};
}

static void pushVec2(lua_State *L, Vec2 v)
{
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
}

static Vec3 toVec3(lua_State *L, int n)
{
	double x = luaL_checknumber(L, n + 0);
	double y = luaL_checknumber(L, n + 1);
	double z = luaL_checknumber(L, n + 2);

	return (Vec3){x, y, z};
}

static void pushVec3(lua_State *L, Vec3 v)
{
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
}

static Vec4 toVec4(lua_State *L, int n)
{
	double x = luaL_checknumber(L, n + 0);
	double y = luaL_checknumber(L, n + 1);
	double z = luaL_checknumber(L, n + 2);
	double w = luaL_checknumber(L, n + 3);

	return (Vec4){x, y, z, w};
}

static void pushVec4(lua_State *L, Vec4 v)
{
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	lua_pushnumber(L, v.w);
}

static Box2 toBox2(lua_State *L, int n)
{
	Vec2 min = toVec2(L, n + 0);
	Vec2 max = toVec2(L, n + 2);

	return (Box2){min, max};
}

static void pushBox2(lua_State *L, Box2 b)
{
	pushVec2(L, b.min);
	pushVec2(L, b.max);
}

static String tonewString(lua_State *L, int valueArg, char *oldValue)
{
	size_t length;
	const char *luaValue = luaL_checklstring(L, valueArg, &length);
	char *newValue = NULL;

	AlError error = al_malloc(&newValue, sizeof(char), length + 1);
	if (error) {
		luaL_error(L, "error setting string value");
	}

	strcpy(newValue, luaValue);
	free(oldValue);

	return newValue;
}

static void *get_instance_ptr(lua_State *L, AlVarReg *entry, int n)
{
	luaL_checkany(L, n);
	switch (lua_type(L, n)) {
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
			return lua_touserdata(L, n) + entry->access.instanceOffset;

		default:
			luaL_error(L, "must supply userdata for this var, %s", entry->name);
			return NULL;
	}
}

#define ACCESSOR(type, get, n, set) \
	static int get_##type##_global(lua_State *L) \
	{ \
		type *ptr = lua_touserdata(L, lua_upvalueindex(1)); \
		get(L, *ptr); \
		return n; \
	} \
	static int get_##type##_instance(lua_State *L) \
	{ \
		AlVarReg *entry = lua_touserdata(L, lua_upvalueindex(1)); \
		type *ptr = get_instance_ptr(L, entry, 1); \
		get(L, *ptr); \
		return n; \
	} \
	static int set_##type##_global(lua_State *L) \
	{ \
		type *ptr = lua_touserdata(L, lua_upvalueindex(1)); \
		const int arg = 1; \
		*ptr = set; \
		return 0; \
	} \
	static int set_##type##_instance(lua_State *L) \
	{ \
		AlVarReg *entry = lua_touserdata(L, lua_upvalueindex(1)); \
		type *ptr = get_instance_ptr(L, entry, 1); \
		const int arg = 2; \
		*ptr = set; \
		lua_pushvalue(L, 1); \
		return 1; \
	}

ACCESSOR(bool, lua_pushboolean, 1, tobool(L, arg))
ACCESSOR(int, lua_pushinteger, 1, luaL_checkint(L, arg))
ACCESSOR(double, lua_pushnumber, 1, luaL_checknumber(L, arg))
ACCESSOR(Vec2, pushVec2, 2, toVec2(L, arg))
ACCESSOR(Vec3, pushVec3, 3, toVec3(L, arg))
ACCESSOR(Vec4, pushVec4, 4, toVec4(L, arg))
ACCESSOR(Box2, pushBox2, 4, toBox2(L, arg))
ACCESSOR(String, lua_pushstring, 1, tonewString(L, arg, *ptr))

static AlVarReg *get_entry(lua_State *L)
{
	AlVars *vars = lua_touserdata(L, lua_upvalueindex(1));
	const char *name = luaL_checkstring(L, 1);

	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	lua_gettable(L, -2);
	AlVarReg *entry = lua_touserdata(L, -1);
	lua_pop(L, 2);

	if (!entry) {
		luaL_error(L, "no such var: '%s'", name);
	}

	return entry;
}

static int cmd_get(lua_State *L)
{
	AlVarReg *entry = get_entry(L);
	void *ptr;

	if (entry->scope == AL_VAR_GLOBAL) {
		ptr = entry->access.globalPtr;

	} else {
		ptr = get_instance_ptr(L, entry, 2);
	}

	switch (entry->type) {
		case VAR_BOOL: lua_pushboolean(L, *(bool *)ptr); return 1;
		case VAR_INT: lua_pushinteger(L, *(int *)ptr); return 1;
		case VAR_DOUBLE: lua_pushnumber(L, *(double *)ptr); return 1;
		case VAR_VEC2: pushVec2(L, *(Vec2 *)ptr); return 2;
		case VAR_VEC3: pushVec3(L, *(Vec3 *)ptr); return 3;
		case VAR_VEC4: pushVec4(L, *(Vec4 *)ptr); return 4;
		case VAR_BOX2: pushBox2(L, *(Box2 *)ptr); return 4;
		case VAR_STRING: lua_pushstring(L, *(char **)ptr); return 1;
		default: return 0;
	}
}

static int cmd_getter(lua_State *L)
{
	AlVarReg *entry = get_entry(L);
	lua_CFunction getGlobal, getInstance;

#define USE(type) \
	getGlobal = get_##type##_global; \
	getInstance = get_##type##_instance;

	switch (entry->type) {
		case VAR_BOOL: USE(bool); break;
		case VAR_INT: USE(int); break;
		case VAR_DOUBLE: USE(double); break;
		case VAR_VEC2: USE(Vec2); break;
		case VAR_VEC3: USE(Vec3); break;
		case VAR_VEC4: USE(Vec4); break;
		case VAR_BOX2: USE(Box2); break;
		case VAR_STRING: USE(String); break;
		default:
			return 0;
	}
#undef USE

	if (entry->scope == AL_VAR_GLOBAL) {
		lua_pushlightuserdata(L, entry->access.globalPtr);
		lua_pushcclosure(L, getGlobal, 1);

	} else if (lua_gettop(L) >= 2) {
		void *ptr = get_instance_ptr(L, entry, 2);
		lua_pushlightuserdata(L, ptr);
		lua_pushcclosure(L, getGlobal, 1);

	} else {
		lua_pushlightuserdata(L, entry);
		lua_pushcclosure(L, getInstance, 1);
	}

	return 1;
}

static int cmd_set(lua_State *L)
{
	AlVarReg *entry = get_entry(L);
	void *value;
	int valueArg;

	if (entry->scope == AL_VAR_GLOBAL) {
		value = entry->access.globalPtr;
		valueArg = 2;

	} else {
		value = get_instance_ptr(L, entry, 2);
		valueArg = 3;
	}

	switch (entry->type) {
		case VAR_BOOL:
			luaL_checkany(L, 2);
			*(bool *)value = lua_toboolean(L, valueArg);
			break;

		case VAR_INT:
			*(int *)value = luaL_checkint(L, valueArg);
			break;

		case VAR_DOUBLE:
			*(double *)value = luaL_checknumber(L, valueArg);
			break;

		case VAR_VEC2:
			*(Vec2 *)value = toVec2(L, valueArg);
			break;

		case VAR_VEC3:
			*(Vec3 *)value = toVec3(L, valueArg);
			break;

		case VAR_VEC4:
			*(Vec4 *)value = toVec4(L, valueArg);
			break;

		case VAR_BOX2:
			*(Box2 *)value = toBox2(L, valueArg);
			break;

		case VAR_STRING:
			*(String *)value = tonewString(L, valueArg, *(String *)value);
			break;
	}

	return 0;
}

static int cmd_setter(lua_State *L)
{
	AlVarReg *entry = get_entry(L);
	lua_CFunction setGlobal, setInstance;

#define USE(type) \
	setGlobal = set_##type##_global; \
	setInstance = set_##type##_instance;

	switch (entry->type) {
		case VAR_BOOL: USE(bool); break;
		case VAR_INT: USE(int); break;
		case VAR_DOUBLE: USE(double); break;
		case VAR_VEC2: USE(Vec2); break;
		case VAR_VEC3: USE(Vec3); break;
		case VAR_VEC4: USE(Vec4); break;
		case VAR_BOX2: USE(Box2); break;
		case VAR_STRING: USE(String); break;
		default:
			return 0;

	}
#undef USE

	if (entry->scope == AL_VAR_GLOBAL) {
		lua_pushlightuserdata(L, entry->access.globalPtr);
		lua_pushcclosure(L, setGlobal, 1);

	} else if (lua_gettop(L) >= 2) {
		void *ptr = get_instance_ptr(L, entry, 2);
		lua_pushlightuserdata(L, ptr);
		lua_pushcclosure(L, setGlobal, 1);

	} else {
		lua_pushlightuserdata(L, entry);
		lua_pushcclosure(L, setInstance, 1);
	}

	return 1;
}

static const luaL_Reg lib[] = {
	{"get", cmd_get},
	{"getter", cmd_getter},
	{"set", cmd_set},
	{"setter", cmd_setter},
	{NULL, NULL}
};

static int luaopen_vars(lua_State *L)
{
	luaL_newlibtable(L, lib);
	lua_pushlightuserdata(L, &varsKey);
	lua_gettable(L, LUA_REGISTRYINDEX);
	luaL_setfuncs(L, lib, 1);

	return 1;
}
