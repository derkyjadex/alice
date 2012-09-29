/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "model_editing.h"
#include "albase/model_shape.h"
#include "albase/lua.h"

static int cmd_model_new(lua_State *L);
static int cmd_model_free(lua_State *L);
static int cmd_model_load(lua_State *L);
static int cmd_model_save(lua_State *L);

static int cmd_model_get_paths(lua_State *L);
static int cmd_model_add_path(lua_State *L);
static int cmd_model_remove_path(lua_State *L);

static int cmd_model_path_get_colour(lua_State *L);
static int cmd_model_path_set_colour(lua_State *L);
static int cmd_model_path_get_points(lua_State *L);
static int cmd_model_path_set_point(lua_State *L);
static int cmd_model_path_add_point(lua_State *L);
static int cmd_model_path_remove_point(lua_State *L);

AlError model_editing_register_commands(AlCommands *commands)
{
	BEGIN()

	TRY(al_commands_register(commands, "model_new", cmd_model_new, NULL));
	TRY(al_commands_register(commands, "model_free", cmd_model_free, NULL));
	TRY(al_commands_register(commands, "model_load", cmd_model_load, NULL));
	TRY(al_commands_register(commands, "model_save", cmd_model_save, NULL));

	TRY(al_commands_register(commands, "model_get_paths", cmd_model_get_paths, NULL));
	TRY(al_commands_register(commands, "model_add_path", cmd_model_add_path, NULL));
	TRY(al_commands_register(commands, "model_remove_path", cmd_model_remove_path, NULL));

	TRY(al_commands_register(commands, "model_path_get_colour", cmd_model_path_get_colour, NULL));
	TRY(al_commands_register(commands, "model_path_set_colour", cmd_model_path_set_colour, NULL));
	TRY(al_commands_register(commands, "model_path_get_points", cmd_model_path_get_points, NULL));
	TRY(al_commands_register(commands, "model_path_set_point", cmd_model_path_set_point, NULL));
	TRY(al_commands_register(commands, "model_path_add_point", cmd_model_path_add_point, NULL));
	TRY(al_commands_register(commands, "model_path_remove_point", cmd_model_path_remove_point, NULL));

	PASS()
}

static void *cmd_accessor(lua_State *L, const char *name, int numArgs)
{
	if (lua_gettop(L) != numArgs) {
		luaL_error(L, "%s: requires %d argument(s)", name, numArgs);
	}

	void *ptr = lua_touserdata(L, -numArgs);
	if (!ptr) {
		luaL_error(L, "%s: first argument cannot be nil", name);
	}

	return ptr;
}

static int cmd_model_new(lua_State *L)
{
	BEGIN()

	AlModelShape *model = NULL;
	TRY(al_model_shape_init(&model));

	lua_pushlightuserdata(L, model);

	CATCH(
		  return luaL_error(L, "Error creating model");
	)
	FINALLY(
		return 1;
	)
}

static int cmd_model_free(lua_State *L)
{
	AlModelShape *model = cmd_accessor(L, "model_free", 1);
	lua_pop(L, 1);

	al_model_shape_free(model);

	return 0;
}

static int cmd_model_load(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_accessor(L, "model_load", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_load(model, filename));
	lua_pop(L, 2);

	CATCH(
		  return luaL_error(L, "Error loading model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_save(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_accessor(L, "model_save", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_save(model, filename));
	lua_pop(L, 2);

	CATCH(
		return luaL_error(L, "Error saving model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_get_paths(lua_State *L)
{
	AlModelShape *model = cmd_accessor(L, "model_get_paths", 1);
	lua_pop(L, 1);

	for (int i = 0; i < model->numPaths; i++) {
		lua_pushlightuserdata(L, &model->paths[i]);
	}

	return model->numPaths;
}

static int cmd_model_add_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_accessor(L, "model_add_path", 6);
	int index = (int)lua_tointeger(L, -5) - 1;
	double startX = lua_tonumber(L, -4);
	double startY = lua_tonumber(L, -3);
	double endX = lua_tonumber(L, -2);
	double endY = lua_tonumber(L, -1);
	lua_pop(L, 6);

	TRY(al_model_shape_add_path(model, index, (Vec2){startX, startY}, (Vec2){endX, endY}));

	CATCH(
		return luaL_error(L, "Error adding path:");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_remove_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_accessor(L, "model_remove_path", 2);
	int index = (int)lua_tointeger(L, -1) - 1;
	lua_pop(L, -1);

	TRY(model_shape_remove_path(model, index));

	CATCH(
		return luaL_error(L, "Error removing path");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_path_get_colour(lua_State *L)
{
	AlModelPath *path = cmd_accessor(L, "model_path_get_colour", 1);
	lua_pop(L, 1);

	lua_pushnumber(L, path->colour.x);
	lua_pushnumber(L, path->colour.y);
	lua_pushnumber(L, path->colour.z);

	return 3;
}

static int cmd_model_path_set_colour(lua_State *L)
{
	AlModelPath *path = cmd_accessor(L, "model_path_set_colour", 4);
	double r = lua_tonumber(L, -3);
	double g = lua_tonumber(L, -2);
	double b = lua_tonumber(L, -1);
	lua_pop(L, 4);

	path->colour = (Vec3){r, g, b};

	return 0;
}

static int cmd_model_path_get_points(lua_State *L)
{
	AlModelPath *path = cmd_accessor(L, "model_path_get_points", 1);
	lua_pop(L, 1);

	for (int i = 0; i < path->numPoints; i++) {
		lua_pushnumber(L, path->points[i].x);
		lua_pushnumber(L, path->points[i].y);
	}

	return path->numPoints * 2;
}

static int cmd_model_path_set_point(lua_State *L)
{
	AlModelPath *path = cmd_accessor(L, "model_path_set_point", 4);
	int index = (int)lua_tointeger(L, -3) - 1;
	double x = lua_tonumber(L, -2);
	double y = lua_tonumber(L, -1);
	lua_pop(L, 3);

	path->points[index] = (Vec2){x, y};

	return 0;
}

static int cmd_model_path_add_point(lua_State *L)
{
	BEGIN()

	AlModelPath *path = cmd_accessor(L, "model_path_add_point", 4);
	int index = (int)lua_tointeger(L, -3) - 1;
	double x = lua_tonumber(L, -2);
	double y = lua_tonumber(L, -1);
	lua_pop(L, 4);

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

	AlModelPath *path = cmd_accessor(L, "model_path_add_point", 2);
	int index = (int)lua_tointeger(L, -1) - 1;
	lua_pop(L, 2);

	TRY(al_model_path_remove_point(path, index));

	CATCH(
		return luaL_error(L, "Error removing point from path");
	)
	FINALLY(
		return 0;
	)
}
