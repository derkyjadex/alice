/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "albase/model_shape.h"
#include "albase/stream.h"
#include "albase/data.h"
#include "albase/wrapper.h"
#include "model_shape_internal.h"
#include "model_shape_cmds.h"

#define SHAPE_TAG AL_DATA_TAG('S', 'H', 'A', 'P')
#define PATHS_TAG AL_DATA_TAG('P', 'T', 'H', 'S')
#define COLOUR_TAG AL_DATA_TAG('C', 'O', 'L', 'R')
#define POINTS_TAG AL_DATA_TAG('P', 'N', 'T', 'S')

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

static AlError al_model_path_load(AlModelPath *path, AlData *data)
{
	BEGIN()

	Vec3 colour;
	Vec2 *points = NULL;
	uint64_t numPoints = 0;

	TRY(al_data_read_start(data, NULL));

	READ_TAGS(data, {
		CASE_TAG(COLOUR_TAG, {
			TRY(al_data_read_value(data, AL_VAR_VEC3, &colour, NULL));
			TRY(al_data_skip_rest(data));
		})
		CASE_TAG(POINTS_TAG, {
			if (points)
				THROW(AL_ERROR_INVALID_DATA);

			TRY(al_data_read_array(data, AL_VAR_VEC2, &points, &numPoints, NULL));
			if (numPoints > INT_MAX)
				THROW(AL_ERROR_INVALID_DATA);

			TRY(al_data_skip_rest(data));
		})
	})

	if (!points)
		THROW(AL_ERROR_INVALID_DATA);

	free(path->points);

	path->colour = colour;
	path->pointsLength = numPoints;
	path->numPoints = (int)numPoints;
	path->points = points;

	CATCH(
		free(points);
	)
	FINALLY()
}

static AlError al_model_path_save(AlModelPath *path, AlData *data)
{
	BEGIN()

	TRY(al_data_write_start(data));

	TRY(al_data_write_simple_tag(data, COLOUR_TAG, AL_VAR_VEC3, &path->colour));

	TRY(al_data_write_start_tag(data, POINTS_TAG));
	TRY(al_data_write_array(data, AL_VAR_VEC2, path->points, path->numPoints));
	TRY(al_data_write_end(data));

	TRY(al_data_write_end(data));

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

AlError al_model_shape_load(AlModelShape *shape, AlStream *stream)
{
	BEGIN()

	AlData *data = NULL;

	int numPaths = 0;
	AlModelPath **paths = NULL;

	TRY(al_data_init(&data, stream));
	TRY(al_data_read_start_tag(data, SHAPE_TAG, NULL));

	READ_TAGS(data, {
		CASE_TAG(PATHS_TAG, {
			if (paths)
				THROW(AL_ERROR_INVALID_DATA)

			TRY(al_data_read_value(data, AL_VAR_INT, &numPaths, NULL));
			TRY(al_malloc(&paths, sizeof(AlModelPath *), numPaths));

			for (int i = 0; i < numPaths; i++) {
				paths[i] = NULL;

				TRY(al_wrapper_invoke_ctor(pathWrapper, &paths[i]));
				reference(shape, paths[i]);
				TRY(al_model_path_load(paths[i], data));
			}

			TRY(al_data_skip_rest(data));
		})
	})

	if (!paths)
		THROW(AL_ERROR_INVALID_DATA);

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
		al_data_free(data);
	)
}

AlModelPath *const *al_model_shape_get_paths(AlModelShape *shape, int *numPaths)
{
	if (numPaths) {
		*numPaths = shape->numPaths;
	}

	return shape->paths;
}

AlError al_model_shape_save(AlModelShape *shape, AlStream *stream)
{
	BEGIN()

	AlData *data = NULL;

	TRY(al_data_init(&data, stream));

	TRY(al_data_write_start_tag(data, SHAPE_TAG));
	TRY(al_data_write_start_tag(data, PATHS_TAG));
	TRY(al_data_write_value(data, AL_VAR_INT, &shape->numPaths));

	for (int i = 0; i < shape->numPaths; i++) {
		TRY(al_model_path_save(shape->paths[i], data));
	}

	TRY(al_data_write_end(data));
	TRY(al_data_write_end(data));

	PASS(
		al_data_free(data);
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

static bool inside_triangle(Vec2 a, Vec2 b, Vec2 c, Vec2 p)
{
	return vec2_cross(a, b, p) * vec2_cross(a, b, c) >= 0 &&
		   vec2_cross(b, c, p) * vec2_cross(b, c, a) >= 0 &&
		   vec2_cross(c, a, p) * vec2_cross(c, a, b) >= 0;
}

static bool crosses_infront(Vec2 a, Vec2 c, Vec2 p)
{
	if ((a.y > p.y && c.y > p.y) ||
		(a.y <= p.y && c.y <= p.y) ||
		(a.x <= p.x && c.x <= p.x))
		return false;

	if (a.x >= p.x && c.x >= p.x) {
		return !vec2_equals(a, p) && !vec2_equals(c, p);
	}

	double a1 = (c.y - a.y) / (c.x - a.x);
	double a0 = c.y - (a1 * c.x);

	return (p.y - a0) / a1 > p.x;
}

static bool inside_curve(Vec2 a, Vec2 b, Vec2 c, Vec2 p)
{
	double det = vec2_cross(a, c, b);
	double l3 = vec2_cross(a, p, b) / det;
	double l2 = vec2_cross(a, c, p) / det;

	double x = l2 * 0.5 + l3;
	double y = l3;

	return x * x - y <= 0;
}

bool al_model_path_hit_test(AlModelPath *path, Vec2 point)
{
	bool result = false;

	int n = path->numPoints;
	Vec2 *points = path->points;

	for (int i = 0; i < n - 1; i += 2) {
		bool last = i == n - 2;

		Vec2 a = points[i + 0];
		Vec2 b = points[i + 1];
		Vec2 c = points[last ? 0 : i + 2];

		if ((a.y > point.y && b.y > point.y && c.y > point.y) ||
			(a.y < point.y && b.y < point.y && c.y < point.y) ||
			(a.x < point.x && b.x < point.x && c.x < point.x))
			continue;

		bool insideTriangle = inside_triangle(a, b, c, point);
		bool crossesInfront = crosses_infront(a, c, point);

		if (!insideTriangle) {
			if (crossesInfront) {
				result = !result;
			}
		} else {
			bool insideCurve = inside_curve(a, b, c, point);

			if (insideCurve != crossesInfront) {
				result = !result;
			}
		}
	}

	return result;
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

AlError al_model_systems_init(lua_State *L, AlVars *vars)
{
	BEGIN()

	TRY(al_wrapper_init(&shapeWrapper, "model_shape", sizeof(AlModelShape), wrapper_model_shape_free));
	TRY(al_wrapper_init(&pathWrapper, "model_path", sizeof(AlModelPath), wrapper_model_path_free));

	TRY(al_wrapper_wrap_ctor(shapeWrapper, al_model_shape_ctor));
	TRY(al_wrapper_wrap_ctor(pathWrapper, al_model_path_ctor));

	luaL_requiref(L, "model", luaopen_model, false);
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
