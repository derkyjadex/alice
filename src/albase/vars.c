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
		void *value;
		size_t offset;
	} data;
} AlVarEntry;

struct AlVars {
	lua_State *lua;
	AlLuaKey entries;
};

static int cmd_get(lua_State *L);
static int cmd_set(lua_State *L);

AlError al_vars_init(AlVars **result, lua_State *lua, AlCommands *commands)
{
	BEGIN()

	AlVars *vars = NULL;
	TRY(al_malloc(&vars, sizeof(AlVars), 1));

	vars->lua = lua;

	lua_pushlightuserdata(lua, &vars->entries);
	lua_newtable(lua);
	lua_settable(lua, LUA_REGISTRYINDEX);

	TRY(al_commands_register(commands, "get", cmd_get, vars));
	TRY(al_commands_register(commands, "set", cmd_set, vars));

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

static AlError vars_register(AlVars *vars, const char *name, AlVarType type, bool global, void *value, size_t offset)
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
		entry->data.value = value;
	} else {
		entry->data.offset = offset;
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

AlError al_vars_register_global(AlVars *vars, const char *name, AlVarType type, void *value)
{
	return vars_register(vars, name, type, true, value, 0);
}

AlError al_vars_register_instance(AlVars *vars, const char *name, AlVarType type, size_t offset)
{
	return vars_register(vars, name, type, false, NULL, offset);
}

static int cmd_get(lua_State *L)
{
	AlVars *vars = lua_touserdata(L, lua_upvalueindex(1));
	const char *name = luaL_checkstring(L, 1);

	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	lua_gettable(L, -2);
	AlVarEntry *entry = lua_touserdata(L, -1);

	if (entry == NULL) {
		return luaL_error(L, "get: no such var: '%s'", name);
	}

	void *value;

	if (entry->global) {
		value = entry->data.value;

	} else {
		luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
		value = lua_touserdata(L, 2) + entry->data.offset;
	}

	switch (entry->type) {
		case VAR_BOOL:
			lua_pushboolean(L, *(bool *)value);
			return 1;

		case VAR_INT:
			lua_pushinteger(L, *(int *)value);
			return 1;

		case VAR_DOUBLE:
			lua_pushnumber(L, *(double *)value);
			return 1;

		case VAR_VEC2:
			lua_pushnumber(L, (*(Vec2 *)value).x);
			lua_pushnumber(L, (*(Vec2 *)value).y);
			return 2;

		case VAR_VEC3:
			lua_pushnumber(L, (*(Vec3 *)value).x);
			lua_pushnumber(L, (*(Vec3 *)value).y);
			lua_pushnumber(L, (*(Vec3 *)value).z);
			return 3;

		case VAR_VEC4:
			lua_pushnumber(L, (*(Vec4 *)value).x);
			lua_pushnumber(L, (*(Vec4 *)value).y);
			lua_pushnumber(L, (*(Vec4 *)value).z);
			lua_pushnumber(L, (*(Vec4 *)value).w);
			return 4;

		case VAR_BOX:
			lua_pushnumber(L, (*(Box *)value).min.x);
			lua_pushnumber(L, (*(Box *)value).min.y);
			lua_pushnumber(L, (*(Box *)value).max.x);
			lua_pushnumber(L, (*(Box *)value).max.y);
			return 4;

		case VAR_STRING:
			lua_pushstring(L, *(char **)value);
			return 1;

		default:
			return 0;
	}
}

static AlError cmd_set_string(lua_State *L, int valueArg, char **value)
{
	BEGIN()

	size_t length;
	const char *luaValue = luaL_checklstring(L, valueArg, &length);
	char *newValue = NULL;

	TRY(al_malloc(&newValue, sizeof(char), length + 1));
	strcpy(newValue, luaValue);

	free(*value);

	*value = newValue;

	PASS()
}

static int cmd_set(lua_State *L)
{
	AlVars *vars = lua_touserdata(L, lua_upvalueindex(1));
	const char *name = luaL_checkstring(L, 1);

	lua_pushlightuserdata(L, &vars->entries);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	lua_gettable(L, -2);
	AlVarEntry *entry = lua_touserdata(L, -1);

	if (entry == NULL) {
		return luaL_error(L, "set: no such var: '%s'", name);
	}

	void *value;
	int valueArg;

	if (entry->global) {
		value = entry->data.value;
		valueArg = 2;
	} else {
		luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
		value = lua_touserdata(L, 2) + entry->data.offset;
		valueArg = 3;
	}

	double x, y, z, w;

	switch (entry->type) {
		case VAR_BOOL:
			luaL_checkany(L, 2);
			*(bool *)value = lua_toboolean(L, valueArg + 0);
			break;

		case VAR_INT:
			*(int *)value = luaL_checkint(L, valueArg + 0);
			break;

		case VAR_DOUBLE:
			*(double *)value = luaL_checknumber(L, valueArg + 0);
			break;

		case VAR_VEC2:
			x = luaL_checknumber(L, valueArg + 0);
			y = luaL_checknumber(L, valueArg + 1);
			*(Vec2 *)value = (Vec2){x, y};
			break;

		case VAR_VEC3:
			x = luaL_checknumber(L, valueArg + 0);
			y = luaL_checknumber(L, valueArg + 1);
			z = luaL_checknumber(L, valueArg + 2);
			*(Vec3 *)value = (Vec3){x, y, z};
			break;

		case VAR_VEC4:
			x = luaL_checknumber(L, valueArg + 0);
			y = luaL_checknumber(L, valueArg + 1);
			z = luaL_checknumber(L, valueArg + 2);
			w = luaL_checknumber(L, valueArg + 3);
			*(Vec4 *)value = (Vec4){x, y, z, w};
			break;

		case VAR_BOX:
			x = luaL_checknumber(L, valueArg + 0);
			y = luaL_checknumber(L, valueArg + 1);
			z = luaL_checknumber(L, valueArg + 2);
			w = luaL_checknumber(L, valueArg + 3);
			*(Box *)value = (Box){{x, y}, {z, w}};
			break;

		case VAR_STRING:
			if (cmd_set_string(L, valueArg, (char **)value) != AL_NO_ERROR) {
				return luaL_error(L, "set: error setting string value");
			}
			break;
	}

	return 0;
}
