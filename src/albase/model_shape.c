/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "albase/model_shape.h"
#include "albase/file.h"
#include "albase/wrapper.h"
#include "albase/commands.h"
#include "model_shape_internal.h"
#include "model_shape_cmds.h"

static AlWrapper *shapeWrapper = NULL;
static AlWrapper *pathWrapper = NULL;

static AlError _al_model_path_init(AlModelPath *path)
{
	BEGIN()

	path->colour = (Vec3){1, 1, 1};
	path->numPoints = 0;
	path->pointsLength = 0;
	path->points = NULL;

	TRY(al_malloc(&path->points, sizeof(Vec2), 4));
	path->pointsLength = 4;

	PASS()
}

static int al_model_path_ctor(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_call(L, 0, 1);
	AlModelPath *path = lua_touserdata(L, -1);

	_al_model_path_init(path);

	return 1;
}

static void _al_model_path_free(AlModelPath *path)
{
	if (path) {
		free(path->points);
	}
}

static AlError al_model_path_load(AlModelPath *path, FILE *file)
{
	BEGIN()

	Vec3 colour;
	Vec2 *points = NULL;
	int numPoints = 0;

	TRY(al_file_read(file, &colour, sizeof(Vec3), 1));
	TRY(al_file_read_array(file, &points, &numPoints, sizeof(Vec2)));

	free(path->points);

	path->colour = colour;
	path->pointsLength= numPoints;
	path->numPoints = numPoints;
	path->points = points;

	CATCH(
		free(points);
	)
	FINALLY()
}

static AlError al_model_path_save(AlModelPath *path, FILE *file)
{
	BEGIN()

	TRY(al_file_write(file, &path->colour, sizeof(Vec3), 1));
	TRY(al_file_write_array(file, path->points, path->numPoints, sizeof(Vec2)));

	PASS()
}

static AlError _al_model_shape_init(AlModelShape *shape)
{
	BEGIN()

	shape->numPaths = 0;
	shape->pathsLength = 0;
	shape->paths = NULL;

	TRY(al_malloc(&shape->paths, sizeof(AlModelPath *), 4));
	shape->pathsLength = 4;

	CATCH(
		al_model_shape_free(shape);
	)
	FINALLY()
}

static int al_model_shape_ctor(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_call(L, 0, 1);
	AlModelShape *shape = lua_touserdata(L, -1);

	_al_model_shape_init(shape);

	return 1;
}

AlError al_model_shape_init(AlModelShape **result)
{
	BEGIN()

	AlModelShape *shape= NULL;
	TRY(al_wrapper_invoke_ctor(shapeWrapper, &shape));
	al_wrapper_retain(shapeWrapper, shape);

	*result = shape;

	PASS()
}

static void _al_model_shape_free(AlModelShape *shape)
{
	if (shape) {
		free(shape->paths);
	}
}

void al_model_shape_free(AlModelShape *shape)
{
	al_wrapper_release(shapeWrapper, shape);
}

static void reference(AlModelShape *shape, AlModelPath *path)
{
	al_wrapper_push_userdata(shapeWrapper, shape);
	al_wrapper_push_userdata(pathWrapper, path);
	al_wrapper_reference(shapeWrapper);

	al_wrapper_push_userdata(pathWrapper, path);
	al_wrapper_push_userdata(shapeWrapper, shape);
	al_wrapper_reference(pathWrapper);
}

static void unreference(AlModelShape *shape, AlModelPath *path)
{
	al_wrapper_push_userdata(shapeWrapper, shape);
	al_wrapper_push_userdata(pathWrapper, path);
	al_wrapper_unreference(shapeWrapper);

	al_wrapper_push_userdata(pathWrapper, path);
	al_wrapper_push_userdata(shapeWrapper, shape);
	al_wrapper_unreference(pathWrapper);
}

AlError al_model_shape_load(AlModelShape *shape, const char *filename)
{
	BEGIN()

	FILE *file = NULL;
	int numPaths = 0;
	AlModelPath **paths = NULL;

	TRY(al_file_open(&file, filename, OPEN_READ));
	TRY(al_file_read(file, &numPaths, sizeof(int), 1));
	TRY(al_malloc(&paths, sizeof(AlModelPath *), numPaths));

	for (int i = 0; i < numPaths; i++) {
		paths[i] = NULL;
		TRY(al_wrapper_invoke_ctor(pathWrapper, &paths[i]));
		reference(shape, paths[i]);
		TRY(al_model_path_load(paths[i], file));
	}

	for (int i = 0; i < shape->numPaths; i++) {
		unreference(shape, shape->paths[i]);
	}

	free(shape->paths);

	shape->numPaths = numPaths;
	shape->pathsLength = numPaths;
	shape->paths = paths;

	CATCH(
		al_log_error("Error reading model file");
		if (paths) {
			for (int i = 0; i < numPaths; i++) {
			  if (!paths[i])
				  break;

			  unreference(shape, paths[i]);
			}

			free(paths);
		}
	)
	FINALLY(
		al_file_close(file);
	)
}

AlModelPath *const *al_model_shape_get_paths(AlModelShape *shape, int *numPaths)
{
	if (numPaths) {
		*numPaths = shape->numPaths;
	}

	return shape->paths;
}

AlError al_model_shape_save(AlModelShape *shape, const char *filename)
{
	BEGIN()

	FILE *file = NULL;

	TRY(al_file_open(&file, filename, OPEN_WRITE));
	TRY(al_file_write(file, &shape->numPaths, sizeof(int), 1));

	for (int i = 0; i < shape->numPaths; i++) {
		TRY(al_model_path_save(shape->paths[i], file));
	}

	PASS(
		al_file_close(file);
	)
}

AlError al_model_shape_add_path(AlModelShape *shape, int index, Vec2 start, Vec2 end)
{
	assert(index >= -1 && index <= shape->numPaths);

	BEGIN()

	AlModelPath *path = NULL;

	if (index == -1) {
		index = shape->numPaths;
	}

	if (shape->numPaths == shape->pathsLength) {
		TRY(al_realloc(&shape->paths, sizeof(AlModelPath), shape->pathsLength * 2));
		shape->pathsLength *= 2;
	}

	TRY(al_wrapper_invoke_ctor(pathWrapper, &path));
	path->points[0] = start;
	path->points[1] = end;
	path->numPoints = 2;

	for (int i = shape->numPaths; i > index; i--) {
		shape->paths[i] = shape->paths[i - 1];
	}

	shape->paths[index] = path;

	reference(shape, path);

	shape->numPaths++;

	PASS()
}

AlError al_model_shape_remove_path(AlModelShape *shape, int index)
{
	assert(index >= 0 && index < shape->numPaths);

	AlModelPath *path = shape->paths[index];

	unreference(shape, path);

	for (int i = index; i < shape->numPaths; i++) {
		shape->paths[i] = shape->paths[i + 1];
	}

	shape->numPaths--;

	return AL_NO_ERROR;
}

Vec3 al_model_path_get_colour(AlModelPath *path)
{
	return path->colour;
}

void al_model_path_set_colour(AlModelPath *path, Vec3 colour)
{
	path->colour = colour;
}

Vec2 *al_model_path_get_points(AlModelPath *path, int *numPoints)
{
	if (numPoints) {
		*numPoints = path->numPoints;
	}

	return path->points;
}

AlError al_model_path_add_point(AlModelPath *path, int index, Vec2 location)
{
	assert(index >= -1 && index <= path->numPoints);

	BEGIN()

	if (index == -1)
		index = path->numPoints;

	if (path->numPoints == path->pointsLength) {
		TRY(al_realloc(&path->points, sizeof(Vec2), path->pointsLength * 2));
		path->pointsLength *= 2;
	}

	for (int i = path->numPoints; i > index; i--) {
		path->points[i] = path->points[i - 1];
	}

	path->points[index] = location;

	path->numPoints++;

	PASS()
}

AlError al_model_path_remove_point(AlModelPath *path, int index)
{
	assert(index >= 0 && index < path->numPoints);
	assert(path->numPoints > 2);

	for (int i = index; i < path->numPoints; i++) {
		path->points[i] = path->points[i + 1];
	}

	path->numPoints--;

	return AL_NO_ERROR;
}

void al_model_shape_push_userdata(AlModelShape *shape)
{
	al_wrapper_push_userdata(shapeWrapper, shape);
}

void al_model_path_push_userdata(AlModelPath *path)
{
	al_wrapper_push_userdata(pathWrapper, path);
}

static void wrapper_model_shape_free(lua_State *L, void *ptr)
{
	_al_model_shape_free(ptr);
}

static void wrapper_model_path_free(lua_State *L, void *ptr)
{
	_al_model_path_free(ptr);
}

bool al_model_path_hit_test(AlModelPath *path, Vec2 point)
{
	// TODO: This is incorrect right now, doesn't do the curves properly
	bool result = false;

	for (int i = 0, j = path->numPoints - 1; i < path->numPoints; j = i++) {
		Vec2 p1 = path->points[i];
		Vec2 p2 = path->points[j];

		if ((p1.y > point.y && p2.y > point.y) ||
			(p1.y <= point.y && p2.y <= point.y) ||
			(p1.x <= point.x && p2.x <= point.x))
			continue;

		if (p1.x >= point.x && p2.x >= point.x) {
			if (!vec2_equals(p1, point) && !vec2_equals(p2, point)) {
				result = !result;

				continue;
			}

			double a = (p2.y - p1.y) / (p2.x - p1.x);
			double b = p2.y - (a * p2.x);

			if ((point.y - b) / a > point.x)
				result = !result;
		}
	}

	return result;
}

AlError al_model_systems_init(lua_State *L, AlCommands *commands, AlVars *vars)
{
	BEGIN()

	TRY(al_wrapper_init(&shapeWrapper, L, sizeof(AlModelShape), wrapper_model_shape_free));
	TRY(al_wrapper_init(&pathWrapper, L, sizeof(AlModelPath), wrapper_model_path_free));

	TRY(al_wrapper_wrap_ctor(shapeWrapper, al_model_shape_ctor));
	TRY(al_wrapper_wrap_ctor(pathWrapper, al_model_path_ctor));

	TRY(al_wrapper_register_commands(shapeWrapper, commands, "model_shape"));
	TRY(al_wrapper_register_commands(pathWrapper, commands, "model_path"));

	TRY(al_model_commands_init(commands));
	TRY(al_model_vars_init(vars));

	PASS()
}

void al_model_systems_free()
{
	al_wrapper_free(shapeWrapper);
	shapeWrapper = NULL;

	al_wrapper_free(pathWrapper);
	pathWrapper = NULL;
}
