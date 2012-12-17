/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "model_commands.h"
#include "albase/model_shape.h"
#include "albase/lua.h"

static AlModelShape *cmd_model_shape_accessor(lua_State *L, const char *name, int numArgs)
{
	if (lua_gettop(L) != numArgs) {
		luaL_error(L, "model_shape_%s: requires %d argument(s)", name, numArgs);
	}

	return lua_touserdata(L, 1);
}

static int cmd_model_shape_load(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "load", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_load(model, filename));

	CATCH(
		return luaL_error(L, "Error loading model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_shape_save(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "save", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_save(model, filename));

	CATCH(
		return luaL_error(L, "Error saving model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_shape_get_paths(lua_State *L)
{
	AlModelShape *model = cmd_model_shape_accessor(L, "get_paths", 1);

	for (int i = 0; i < model->numPaths; i++) {
		al_model_path_push_userdata(model->paths[i]);
	}

	return model->numPaths;
}

static int cmd_model_shape_add_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "add_path", 6);
	int index = (int)lua_tointeger(L, -5) - 1;
	double startX = lua_tonumber(L, -4);
	double startY = lua_tonumber(L, -3);
	double endX = lua_tonumber(L, -2);
	double endY = lua_tonumber(L, -1);

	TRY(al_model_shape_add_path(model, index, (Vec2){startX, startY}, (Vec2){endX, endY}));

	CATCH(
		return luaL_error(L, "Error adding path:");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_shape_remove_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "remove_path", 2);
	int index = (int)lua_tointeger(L, -1) - 1;

	TRY(al_model_shape_remove_path(model, index));

	CATCH(
		return luaL_error(L, "Error removing path");
	)
	FINALLY(
		return 0;
	)
}

static AlModelPath *cmd_path_accessor(lua_State *L, const char *name, int numArgs)
{
	if (lua_gettop(L) != numArgs) {
		luaL_error(L, "model_path_%s: requires %d argument(s)", name, numArgs);
	}

	return lua_touserdata(L, 1);
}

static int cmd_model_path_get_points(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "get_points", 1);

	for (int i = 0; i < path->numPoints; i++) {
		lua_pushnumber(L, path->points[i].x);
		lua_pushnumber(L, path->points[i].y);
	}

	return path->numPoints * 2;
}

static int cmd_model_path_set_point(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "set_point", 4);
	int index = (int)lua_tointeger(L, -3) - 1;
	double x = lua_tonumber(L, -2);
	double y = lua_tonumber(L, -1);

	path->points[index] = (Vec2){x, y};

	return 0;
}

static int cmd_model_path_add_point(lua_State *L)
{
	BEGIN()

	AlModelPath *path = cmd_path_accessor(L, "add_point", 4);
	int index = (int)lua_tointeger(L, -3) - 1;
	double x = lua_tonumber(L, -2);
	double y = lua_tonumber(L, -1);

	TRY(al_model_path_add_point(path, index, (Vec2){x, y}));

	CATCH(
		return luaL_error(L, "Error adding point to path");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_path_remove_point(lua_State *L)
{
	BEGIN()

	AlModelPath *path = cmd_path_accessor(L, "add_point", 2);
	int index = (int)lua_tointeger(L, -1) - 1;

	TRY(al_model_path_remove_point(path, index));

	CATCH(
		return luaL_error(L, "Error removing point from path");
	)
	FINALLY(
		return 0;
	)
}

#define REG_SHAPE_CMD(x) TRY(al_commands_register(commands, "model_shape_"#x, cmd_model_shape_ ## x, NULL))
#define REG_PATH_CMD(x) TRY(al_commands_register(commands, "model_path_"#x, cmd_model_path_ ## x, NULL))

AlError al_model_commands_init(AlCommands *commands)
{
	BEGIN()

	REG_SHAPE_CMD(load);
	REG_SHAPE_CMD(save);

	REG_SHAPE_CMD(get_paths);
	REG_SHAPE_CMD(add_path);
	REG_SHAPE_CMD(remove_path);

	REG_PATH_CMD(get_points);
	REG_PATH_CMD(set_point);
	REG_PATH_CMD(add_point);
	REG_PATH_CMD(remove_point);

	PASS()
}

AlError al_model_vars_init(AlVars *vars)
{
	BEGIN()

	TRY(al_vars_register_instance(vars, "model_path_colour", VAR_VEC3, offsetof(AlModelPath, colour)));

	PASS()
}