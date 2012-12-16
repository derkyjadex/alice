/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "model_editing.h"
#include "albase/model_shape.h"
#include "albase/lua.h"
#include "albase/wrapper.h"

static AlWrapper *modelWrapper = NULL;
static AlWrapper *pathWrapper = NULL;

AlModelShape *model_editing_unwrap(lua_State *L)
{
	return al_wrapper_unwrap(modelWrapper);
}

static AlModelShape *cmd_model_accessor(lua_State *L, const char *name, int numArgs)
{
	if (lua_gettop(L) != numArgs) {
		luaL_error(L, "model_%s: requires %d argument(s)", name, numArgs);
	}

	lua_pushvalue(L, 1);
	return al_wrapper_unwrap(modelWrapper);
}

static int cmd_model_new(lua_State *L)
{
	BEGIN()

	luaL_checktype(L, 1, LUA_TTABLE);

	AlModelShape *model = NULL;
	TRY(al_model_shape_init(&model));
	TRY(al_wrapper_register(modelWrapper, model, 1));

	CATCH(
		al_model_shape_free(model);
		return luaL_error(L, "Error creating model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_load(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_accessor(L, "load", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_load(model, filename));

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

	AlModelShape *model = cmd_model_accessor(L, "save", 2);
	const char *filename = lua_tostring(L, -1);

	TRY(al_model_shape_save(model, filename));

	CATCH(
		return luaL_error(L, "Error saving model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_get_paths(lua_State *L)
{
	AlModelShape *model = cmd_model_accessor(L, "get_paths", 1);

	for (int i = 0; i < model->numPaths; i++) {
		lua_pushvalue(L, 1);
		al_wrapper_wrap(pathWrapper, &model->paths[i], 1);
	}

	return model->numPaths;
}

static int cmd_model_add_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_accessor(L, "add_path", 6);
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

static int cmd_model_remove_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_accessor(L, "remove_path", 2);
	int index = (int)lua_tointeger(L, -1) - 1;

	al_wrapper_unregister(pathWrapper, &model->paths[index]);
	TRY(model_shape_remove_path(model, index));

	CATCH(
		return luaL_error(L, "Error removing path");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_path_register_ctor(lua_State *L)
{
	BEGIN()

	luaL_checkany(L, 1);
	lua_pushvalue(L, 1);
	TRY(al_wrapper_register_ctor(pathWrapper));

	CATCH(
		return luaL_error(L, "Error registering model path constructor");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_model_path_register(lua_State *L)
{
	BEGIN()

	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	AlModelPath *path = lua_touserdata(L, 2);

	TRY(al_wrapper_register(pathWrapper, path, 1));

	CATCH(
		return luaL_error(L, "Error registering model path");
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

	lua_pushvalue(L, 1);
	return al_wrapper_unwrap(pathWrapper);
}

static int cmd_model_path_get_colour(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "get_colour", 1);

	lua_pushnumber(L, path->colour.x);
	lua_pushnumber(L, path->colour.y);
	lua_pushnumber(L, path->colour.z);

	return 3;
}

static int cmd_model_path_set_colour(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "set_colour", 4);
	double r = lua_tonumber(L, -3);
	double g = lua_tonumber(L, -2);
	double b = lua_tonumber(L, -1);

	path->colour = (Vec3){r, g, b};

	return 0;
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

static void wrapper_model_free(lua_State *L, void *ptr)
{
	AlModelShape *model = ptr;

	for (int i = 0; i < model->numPaths; i++) {
		al_wrapper_unregister(pathWrapper, &model->paths[i]);
	}

	al_wrapper_unregister(modelWrapper, model);
	al_model_shape_free(model);
}

static AlError model_system_init_lua(lua_State *L)
{
	BEGIN()

	TRY(al_wrapper_init(&modelWrapper, L, true, wrapper_model_free));
	TRY(al_wrapper_init(&pathWrapper, L, true, NULL));

	PASS()
}

#define REG_MODEL_CMD(x) TRY(al_commands_register(commands, "model_"#x, cmd_model_ ## x, NULL))
#define REG_PATH_CMD(x) TRY(al_commands_register(commands, "model_path_"#x, cmd_model_path_ ## x, NULL))

static AlError model_system_register_commands(AlCommands *commands)
{
	BEGIN()

	REG_MODEL_CMD(new);
	REG_MODEL_CMD(load);
	REG_MODEL_CMD(save);

	REG_MODEL_CMD(get_paths);
	REG_MODEL_CMD(add_path);
	REG_MODEL_CMD(remove_path);

	REG_PATH_CMD(register_ctor);
	REG_PATH_CMD(register);
	REG_PATH_CMD(get_colour);
	REG_PATH_CMD(set_colour);
	REG_PATH_CMD(get_points);
	REG_PATH_CMD(set_point);
	REG_PATH_CMD(add_point);
	REG_PATH_CMD(remove_point);

	PASS()
}

AlError model_system_init(lua_State *L, AlCommands *commands)
{
	BEGIN()

	TRY(model_system_init_lua(L));
	TRY(model_system_register_commands(commands));

	PASS()
}