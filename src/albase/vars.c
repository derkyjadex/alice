/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <string.h>
#include <stdlib.h>

#include "albase/vars.h"
#include "albase/commands.h"
#include "albase/geometry.h"
#include "albase/lua.h"

typedef struct AlVarEntry {
	char *name;
	AlVarType type;
	bool global;
	union {
		struct {
			void *ptr;
		} global;
		struct {
			size_t offset;
		} instance;
	} data;
} AlVarEntry;

struct AlVars {
	lua_State *lua;
	AlLuaKey entries;
};

typedef char * String;

static int cmd_get(lua_State *L);
static int cmd_getter(lua_State *L);
static int cmd_set(lua_State *L);
static int cmd_setter(lua_State *L);

AlError al_vars_init(AlVars **result, lua_State *lua, AlCommands *commands)
{
	BEGIN()

	AlVars *vars = NULL;
	TRY(al_malloc(&vars, sizeof(AlVars), 1));

	vars->lua = lua;

	lua_pushlightuserdata(lua, &vars->entries);
	lua_newtable(lua);
	lua_settable(lua, LUA_REGISTRYINDEX);

	TRY(al_commands_register(commands, "get", cmd_get, vars, NULL));
	TRY(al_commands_register(commands, "getter", cmd_getter, vars, NULL));
	TRY(al_commands_register(commands, "set", cmd_set, vars, NULL));
	TRY(al_commands_register(commands, "setter", cmd_setter, vars, NULL));

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
			AlVarEntry *entry = lua_touserdata(L, -1);
			free(entry->name);
			free(entry);
			lua_pop(L, 1);
		}

		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		free(vars);
	}
}

static AlError vars_register(AlVars *vars, const char *name, AlVarType type, bool global, void *ptr, size_t offset)
{
	BEGIN()

	AlVarEntry *entry = NULL;
	TRY(al_malloc(&entry, sizeof(AlVarEntry), 1));

	entry->name = NULL;
	TRY(al_malloc(&entry->name, sizeof(char), strlen(name) + 1));
	strcpy(entry->name, name);

	entry->type = type;
	entry->global = global;
	if (global) {
		entry->data.global.ptr = ptr;
	} else {
		entry->data.instance.offset = offset;
	}

	lua_State *L = vars->lua;
	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, name);
	lua_pushlightuserdata(L, entry);
	lua_settable(L, -3);
	lua_pop(L, 1);

	CATCH(
		free(entry);
	)
	FINALLY()
}

AlError al_vars_register_global(AlVars *vars, const char *name, AlVarType type, void *ptr)
{
	return vars_register(vars, name, type, true, ptr, 0);
}

AlError al_vars_register_instance(AlVars *vars, const char *name, AlVarType type, size_t offset)
{
	return vars_register(vars, name, type, false, NULL, offset);
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

static Box toBox(lua_State *L, int n)
{
	Vec2 min = toVec2(L, n + 0);
	Vec2 max = toVec2(L, n + 2);

	return (Box){min, max};
}

static void pushBox(lua_State *L, Box b)
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

static void *get_instance_ptr(lua_State *L, AlVarEntry *entry, int n)
{
	luaL_checkany(L, n);
	switch (lua_type(L, n)) {
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
			return lua_touserdata(L, n) + entry->data.instance.offset;

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
		AlVarEntry *entry = lua_touserdata(L, lua_upvalueindex(1)); \
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
		AlVarEntry *entry = lua_touserdata(L, lua_upvalueindex(1)); \
		type *ptr = get_instance_ptr(L, entry, 1); \
		const int arg = 2; \
		*ptr = set; \
		return 0; \
	}

ACCESSOR(bool, lua_pushboolean, 1, tobool(L, arg))
ACCESSOR(int, lua_pushinteger, 1, luaL_checkint(L, arg))
ACCESSOR(double, lua_pushnumber, 1, luaL_checknumber(L, arg))
ACCESSOR(Vec2, pushVec2, 2, toVec2(L, arg))
ACCESSOR(Vec3, pushVec3, 3, toVec3(L, arg))
ACCESSOR(Vec4, pushVec4, 4, toVec4(L, arg))
ACCESSOR(Box, pushBox, 4, toBox(L, arg))
ACCESSOR(String, lua_pushstring, 1, tonewString(L, arg, *ptr))

static AlVarEntry *get_entry(lua_State *L)
{
	AlVars *vars = lua_touserdata(L, lua_upvalueindex(1));
	const char *name = luaL_checkstring(L, 1);

	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	lua_gettable(L, -2);
	AlVarEntry *entry = lua_touserdata(L, -1);
	lua_pop(L, 2);

	if (!entry) {
		luaL_error(L, "no such var: '%s'", name);
	}

	return entry;
}

static int cmd_get(lua_State *L)
{
	AlVarEntry *entry = get_entry(L);
	void *ptr;

	if (entry->global) {
		ptr = entry->data.global.ptr;

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
		case VAR_BOX: pushBox(L, *(Box *)ptr); return 4;
		case VAR_STRING: lua_pushstring(L, *(char **)ptr); return 1;
		default: return 0;
	}
}

static int cmd_getter(lua_State *L)
{
	AlVarEntry *entry = get_entry(L);
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
		case VAR_BOX: USE(Box); break;
		case VAR_STRING: USE(String); break;
		default:
			return 0;
	}
#undef USE

	if (entry->global) {
		lua_pushlightuserdata(L, entry->data.global.ptr);
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
	AlVarEntry *entry = get_entry(L);
	void *value;
	int valueArg;

	if (entry->global) {
		value = entry->data.global.ptr;
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

		case VAR_BOX:
			*(Box *)value = toBox(L, valueArg);
			break;

		case VAR_STRING:
			*(String *)value = tonewString(L, valueArg, *(String *)value);
			break;
	}

	return 0;
}

static int cmd_setter(lua_State *L)
{
	AlVarEntry *entry = get_entry(L);
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
		case VAR_BOX: USE(Box); break;
		case VAR_STRING: USE(String); break;
		default:
			return 0;

	}
#undef USE

	if (entry->global) {
		lua_pushlightuserdata(L, entry->data.global.ptr);
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
