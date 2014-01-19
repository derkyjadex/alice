/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/model_shape.h"
#include "albase/lua.h"
#include "albase/wrapper.h"

#include "model_shape_cmds.h"
#include "model_shape_internal.h"

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
	const char *filename = luaL_checkstring(L, 2);

	AlStream *stream = NULL;

	TRY(al_stream_init_filename(&stream, filename, AL_OPEN_READ));
	TRY(al_model_shape_load(model, stream));

	CATCH_LUA(, "Error loading model")
	FINALLY_LUA(
		al_stream_free(stream);
	, 0)
}

static int cmd_model_shape_save(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "save", 2);
	const char *filename = luaL_checkstring(L, 2);

	AlStream *stream = NULL;

	TRY(al_stream_init_filename(&stream, filename, AL_OPEN_WRITE));
	TRY(al_model_shape_save(model, stream));

	CATCH_LUA(, "Error saving model")
	FINALLY_LUA(
		al_stream_free(stream);
	, 0)
}

static int cmd_model_shape_get_paths(lua_State *L)
{
	AlModelShape *model = cmd_model_shape_accessor(L, "get_paths", 1);

	for (int i = 0; i < model->numPaths; i++) {
		al_wrapper_push_userdata(L, model->paths[i]);
	}

	return model->numPaths;
}

static int cmd_model_shape_add_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "add_path", 8);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	AlModelPoint start = {
		.location = {
			luaL_checknumber(L, 3),
			luaL_checknumber(L, 4)
		},
		.curveBias = luaL_checknumber(L, 5)
	};

	AlModelPoint end = {
		.location = {
			luaL_checknumber(L, 6),
			luaL_checknumber(L, 7)
		},
		.curveBias = luaL_checknumber(L, 8)
	};

	TRY(al_model_shape_add_path(model, index, start, end));

	if (index == -1)
		index = model->numPaths - 1;

	al_wrapper_push_userdata(L, model->paths[index]);

	CATCH_LUA(, "Error adding path")
	FINALLY_LUA(, 1)
}

static int cmd_model_shape_remove_path(lua_State *L)
{
	BEGIN()

	AlModelShape *model = cmd_model_shape_accessor(L, "remove_path", 2);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	TRY(al_model_shape_remove_path(model, index));

	CATCH_LUA(, "Error removing path")
	FINALLY_LUA(, 0)
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

	luaL_checkstack(L, path->numPoints * 3, "not enough stack space for points");

	for (int i = 0; i < path->numPoints; i++) {
		lua_pushnumber(L, path->points[i].location.x);
		lua_pushnumber(L, path->points[i].location.y);
		lua_pushnumber(L, path->points[i].curveBias);
	}

	return path->numPoints * 3;
}

static int cmd_model_path_set_point(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "set_point", 5);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	path->points[index] = (AlModelPoint){
		.location = {
			luaL_checknumber(L, 3),
			luaL_checknumber(L, 4)
		},
		.curveBias = luaL_checknumber(L, 5)
	};

	return 0;
}

static int cmd_model_path_add_point(lua_State *L)
{
	BEGIN()

	AlModelPath *path = cmd_path_accessor(L, "add_point", 5);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	AlModelPoint point = {
		.location = {
			luaL_checknumber(L, 3),
			luaL_checknumber(L, 4)
		},
		.curveBias = luaL_checknumber(L, 5)
	};

	TRY(al_model_path_add_point(path, index, point));

	CATCH_LUA(, "Error adding point to path")
	FINALLY_LUA(, 0)
}

static int cmd_model_path_remove_point(lua_State *L)
{
	BEGIN()

	AlModelPath *path = cmd_path_accessor(L, "add_point", 2);
	int index = (int)luaL_checkinteger(L, 2) - 1;

	TRY(al_model_path_remove_point(path, index));

	CATCH_LUA(, "Error removing point from path")
	FINALLY_LUA(, 0)
}

static int cmd_model_path_hit_test(lua_State *L)
{
	AlModelPath *path = cmd_path_accessor(L, "hit_test", 3);
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);

	bool result = al_model_path_hit_test(path, (Vec2){x, y});
	lua_pushboolean(L, result);

	return 1;
}

AlError al_model_vars_init(lua_State *L)
{
	BEGIN()

	TRY(al_vars_register(L, (AlVarReg){
		.name = "model_path.colour",
		.type = AL_VAR_VEC3,
		.scope = AL_VAR_INSTANCE,
		.access = {
			.instanceOffset = offsetof(AlModelPath, colour)
		}
	}));

	PASS()
}

static const luaL_Reg lib[] = {
	{"shape_load", cmd_model_shape_load},
	{"shape_save", cmd_model_shape_save},
	{"shape_get_paths", cmd_model_shape_get_paths},
	{"shape_add_path", cmd_model_shape_add_path},
	{"shape_remove_path", cmd_model_shape_remove_path},
	{"path_get_points", cmd_model_path_get_points},
	{"path_set_point", cmd_model_path_set_point},
	{"path_add_point", cmd_model_path_add_point},
	{"path_remove_point", cmd_model_path_remove_point},
	{"path_hit_test", cmd_model_path_hit_test},
	{NULL, NULL}
};

int luaopen_model(lua_State *L)
{
	luaL_newlib(L, lib);

	return 1;
}
