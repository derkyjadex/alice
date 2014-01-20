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

static struct {
	lua_State *lua;
	AlWrappedType *shapeType;
	AlWrappedType *pathType;
} modelSystem = {NULL, NULL, NULL};

static AlError al_model_path_ctor(lua_State *L, void *ptr, void *data)
{
	BEGIN()

	AlModelPath *path = ptr;
	path->colour = (Vec3){1, 1, 1};
	path->numPoints = 0;
	path->pointsLength = 0;
	path->points = NULL;

	TRY(al_malloc(&path->points, sizeof(AlModelPoint) * 4));
	path->pointsLength = 4;

	PASS()
}

static void _al_model_path_free(lua_State *L, void *ptr)
{
	AlModelPath *path = ptr;

	if (path) {
		al_free(path->points);
	}
}

static AlError al_model_path_load(AlModelPath *path, AlData *data)
{
	BEGIN()

	Vec3 colour;
	uint64_t numPoints = 0;
	Vec2 *locations = NULL;
	double *biases = NULL;
	AlModelPoint *points = NULL;

	TRY(al_data_read_start(data, NULL));

	START_READ_TAGS(data) {
		case COLOUR_TAG:
			TRY(al_data_read_value(data, AL_VAR_VEC3, &colour, NULL));
			TRY(al_data_skip_rest(data));
			break;

		case POINTS_TAG: {
			if (points) {
				al_log_error("points can only be specified once in path");
				THROW(AL_ERROR_INVALID_DATA);
			}

			TRY(al_data_read_array(data, AL_VAR_VEC2, &locations, &numPoints, NULL));
			if (numPoints > INT_MAX) {
				al_log_error("too many points in path");
				THROW(AL_ERROR_INVALID_DATA);
			}

			TRY(al_malloc(&points, sizeof(AlModelPoint) * numPoints));

			for (int i = 0; i < numPoints; i++) {
				points[i] = ((AlModelPoint){
					.location = locations[i],
					.curveBias = (i % 2) ? 0.5 : 0.0
				});
			}

			bool biasesMissing;
			uint64_t numBiases;
			TRY(al_data_read_array(data, AL_VAR_DOUBLE, &biases, &numBiases, &biasesMissing));
			if (!biasesMissing) {
				for (int i = 0; i < numPoints && i < numBiases; i++) {
					points[i].curveBias = biases[i];
				}

				TRY(al_data_skip_rest(data));
			}
			break;
		}
	} END_READ_TAGS(data);

	if (!points) {
		al_log_error("no points defined in path");
		THROW(AL_ERROR_INVALID_DATA);
	}

	al_free(path->points);

	path->colour = colour;
	path->pointsLength = numPoints;
	path->numPoints = (int)numPoints;
	path->points = points;

	CATCH({
		al_free(points);
	})
	FINALLY({
		al_free(locations);
		al_free(biases);
	})
}

static AlError al_model_path_save(AlModelPath *path, AlData *data)
{
	BEGIN()

	Vec2 *locations = NULL;
	double *biases = NULL;

	TRY(al_malloc(&locations, sizeof(Vec2) * path->numPoints));
	TRY(al_malloc(&biases, sizeof(double) * path->numPoints));

	for (int i = 0; i < path->numPoints; i++) {
		locations[i] = path->points[i].location;
		biases[i] = path->points[i].curveBias;
	}

	TRY(al_data_write_start(data));

	TRY(al_data_write_simple_tag(data, COLOUR_TAG, AL_VAR_VEC3, &path->colour));

	TRY(al_data_write_start_tag(data, POINTS_TAG));
	TRY(al_data_write_array(data, AL_VAR_VEC2, locations, path->numPoints));
	TRY(al_data_write_array(data, AL_VAR_DOUBLE, biases, path->numPoints));
	TRY(al_data_write_end(data));

	TRY(al_data_write_end(data));

	PASS({
		al_free(locations);
		al_free(biases);
	})
}

static AlError al_model_shape_ctor(lua_State *L, void *ptr, void *data)
{
	BEGIN()

	AlModelShape *shape = ptr;
	shape->numPaths = 0;
	shape->pathsLength = 0;
	shape->paths = NULL;

	TRY(al_malloc(&shape->paths, sizeof(AlModelPath *) * 4));
	shape->pathsLength = 4;

	CATCH({
		al_model_shape_free(shape);
	})
	FINALLY()
}

AlError al_model_shape_init(AlModelShape **result)
{
	return al_wrapper_invoke_ctor(modelSystem.shapeType, result);
}

static void _al_model_shape_free(lua_State *L, void *ptr)
{
	AlModelShape *shape = ptr;

	if (shape) {
		al_free(shape->paths);
	}
}

void al_model_shape_free(AlModelShape *shape)
{
	al_wrapper_release(modelSystem.lua, shape);
}

static void reference(AlModelShape *shape, AlModelPath *path)
{
	al_wrapper_push_userdata(modelSystem.lua, shape);
	al_wrapper_push_userdata(modelSystem.lua, path);
	al_wrapper_reference(modelSystem.lua);

	al_wrapper_push_userdata(modelSystem.lua, path);
	al_wrapper_push_userdata(modelSystem.lua, shape);
	al_wrapper_reference(modelSystem.lua);
}

static void unreference(AlModelShape *shape, AlModelPath *path)
{
	al_wrapper_push_userdata(modelSystem.lua, shape);
	al_wrapper_push_userdata(modelSystem.lua, path);
	al_wrapper_unreference(modelSystem.lua);

	al_wrapper_push_userdata(modelSystem.lua, path);
	al_wrapper_push_userdata(modelSystem.lua, shape);
	al_wrapper_unreference(modelSystem.lua);
}

AlError al_model_shape_load(AlModelShape *shape, AlStream *stream)
{
	BEGIN()

	AlData *data = NULL;

	int numPaths = 0;
	AlModelPath **paths = NULL;

	TRY(al_data_init(&data, stream));
	TRY(al_data_read_start_tag(data, SHAPE_TAG, NULL));

	START_READ_TAGS(data) {
		case PATHS_TAG:
			if (paths) {
				al_log_error("paths can only be specified once in shape");
				THROW(AL_ERROR_INVALID_DATA)
			}

			TRY(al_data_read_value(data, AL_VAR_INT, &numPaths, NULL));
			TRY(al_malloc(&paths, sizeof(AlModelPath *) * numPaths));

			for (int i = 0; i < numPaths; i++) {
				paths[i] = NULL;

				TRY(al_wrapper_invoke_ctor(modelSystem.pathType, &paths[i]));
				reference(shape, paths[i]);
				al_wrapper_release(modelSystem.lua, paths[i]);
				TRY(al_model_path_load(paths[i], data));
			}

			TRY(al_data_skip_rest(data));
			break;
	} END_READ_TAGS(data);

	if (!paths) {
		al_log_error("no paths defined in shape");
		THROW(AL_ERROR_INVALID_DATA);
	}

	for (int i = 0; i < shape->numPaths; i++) {
		unreference(shape, shape->paths[i]);
	}

	al_free(shape->paths);

	shape->numPaths = numPaths;
	shape->pathsLength = numPaths;
	shape->paths = paths;

	CATCH({
		al_log_error("error reading model file");
		if (paths) {
			for (int i = 0; i < numPaths; i++) {
			  if (!paths[i])
				  break;

			  unreference(shape, paths[i]);
			}

			al_free(paths);
		}
	})
	FINALLY({
		al_data_free(data);
	})
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

AlError al_model_shape_add_path(AlModelShape *shape, int index, AlModelPoint start, AlModelPoint end)
{
	assert(index >= -1 && index <= shape->numPaths);

	BEGIN()

	AlModelPath *path = NULL;

	if (index == -1) {
		index = shape->numPaths;
	}

	if (shape->numPaths == shape->pathsLength) {
		TRY(al_realloc(&shape->paths, sizeof(AlModelPath) * shape->pathsLength * 2));
		shape->pathsLength *= 2;
	}

	TRY(al_wrapper_invoke_ctor(modelSystem.pathType, &path));
	path->points[0] = start;
	path->points[1] = end;
	path->numPoints = 2;

	for (int i = shape->numPaths; i > index; i--) {
		shape->paths[i] = shape->paths[i - 1];
	}

	shape->paths[index] = path;

	reference(shape, path);
	al_wrapper_release(modelSystem.lua, path);

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

AlModelPoint *al_model_path_get_points(AlModelPath *path, int *numPoints)
{
	if (numPoints) {
		*numPoints = path->numPoints;
	}

	return path->points;
}

AlError al_model_path_add_point(AlModelPath *path, int index, AlModelPoint point)
{
	assert(index >= -1 && index <= path->numPoints);

	BEGIN()

	if (index == -1)
		index = path->numPoints;

	if (path->numPoints == path->pointsLength) {
		TRY(al_realloc(&path->points, sizeof(AlModelPoint) * path->pointsLength * 2));
		path->pointsLength *= 2;
	}

	for (int i = path->numPoints; i > index; i--) {
		path->points[i] = path->points[i - 1];
	}

	path->points[index] = point;

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

static bool crosses_line(Vec2 a, Vec2 c, Vec2 p)
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

static bool crosses_curve(Vec2 a, Vec2 b, Vec2 c, Vec2 p)
{
	if ((a.y > p.y && b.y > p.y && c.y > p.y) ||
		(a.y < p.y && b.y < p.y && c.y < p.y) ||
		(a.x < p.x && b.x < p.x && c.x < p.x))
		return false;

	bool insideTriangle = inside_triangle(a, b, c, p);
	bool crossesInfront = crosses_line(a, c, p);

	if (!insideTriangle) {
		return crossesInfront;

	} else {
		bool insideCurve = inside_curve(a, b, c, p);

		return insideCurve != crossesInfront;
	}
}

bool al_model_path_hit_test(AlModelPath *path, Vec2 point)
{
	bool result = false;

	int n = path->numPoints;
	AlModelPoint *points = path->points;
	AlModelPoint *a = &points[n - 2];
	AlModelPoint *b = &points[n - 1];
	AlModelPoint *c = &points[0];

	while (c < &points[n]) {
		bool crosses;
		int shift;
		Vec2 mab, mbc;

		int type =
			(a->curveBias != 0) << 0 |
			(b->curveBias != 0) << 1 |
			(c->curveBias != 0) << 2;

		switch (type) {
			case 0:
			case 1:
				crosses = crosses_line(a->location, b->location, point);
				shift = 1;
				break;

			case 2:
				crosses = crosses_curve(a->location, b->location, c->location, point);
				shift = 2;
				break;

			case 3:
				mbc = vec2_mix(b->location, c->location, b->curveBias);
				crosses = crosses_curve(a->location, b->location, mbc, point);
				shift = 1;
				break;

			case 4:
				crosses = crosses_line(b->location, c->location, point);
				shift = 2;
				break;

			case 5:
				crosses = false;
				shift = 1;
				break;

			case 6:
				mab = vec2_mix(a->location, b->location, a->curveBias);
				crosses = crosses_curve(mab, b->location, c->location, point);
				shift = 2;
				break;

			case 7:
				mab = vec2_mix(a->location, b->location, a->curveBias);
				mbc = vec2_mix(b->location, c->location, b->curveBias);
				crosses = crosses_curve(mab, b->location, mbc, point);
				shift = 1;
				break;
		}

		if (crosses) {
			result = !result;
		}

		if (shift == 1) {
			a = b;
			b = c;
			c = c + 1;
		} else if (shift == 2) {
			a = c;
			b = c + 1;
			c = c + 2;
		}
	}

	return result;
}

AlError al_model_systems_init(lua_State *L, AlVars *vars)
{
	BEGIN()

	modelSystem.lua = L;

	TRY(al_wrapper_register(L, (AlWrapperReg){
		.name = "model_shape",
		.size = sizeof(AlModelShape),
		.init = al_model_shape_ctor,
		.initData = NULL,
		.free = _al_model_shape_free
	}, &modelSystem.shapeType));

	TRY(al_wrapper_register(L, (AlWrapperReg){
		.name = "model_path",
		.size = sizeof(AlModelPath),
		.init = al_model_path_ctor,
		.initData = NULL,
		.free = _al_model_path_free
	}, &modelSystem.pathType));

	luaL_requiref(L, "model", luaopen_model, false);
	TRY(al_model_vars_init(vars));

	PASS()
}

void al_model_systems_free()
{
	modelSystem.lua = NULL;
	modelSystem.shapeType = NULL;
	modelSystem.pathType = NULL;
}
