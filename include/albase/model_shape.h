/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_MODEL_SHAPE_H__
#define __ALBASE_MODEL_SHAPE_H__

#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/lua.h"
#include "albase/vars.h"
#include "albase/stream.h"

typedef struct AlModelShape AlModelShape;
typedef struct AlModelPath AlModelPath;

typedef struct {
	Vec2 location;
	double curveBias;
} AlModelPoint;

AlError al_model_systems_init(lua_State *L, AlVars *vars);
void al_model_systems_free(void);

AlError al_model_shape_init(AlModelShape **shape);
void al_model_shape_free(AlModelShape *shape);

AlError al_model_shape_load(AlModelShape *shape, AlStream *stream);
AlError al_model_shape_save(AlModelShape *shape, AlStream *stream);

AlModelPath *const *al_model_shape_get_paths(AlModelShape *shape, int *numPaths);
AlError al_model_shape_add_path(AlModelShape *shape, int index, AlModelPoint start, AlModelPoint end);
AlError al_model_shape_remove_path(AlModelShape *shape, int index);

Vec3 al_model_path_get_colour(AlModelPath *path);
void al_model_path_set_colour(AlModelPath *path, Vec3 colour);
AlModelPoint *al_model_path_get_points(AlModelPath *path, int *numPoints);
AlError al_model_path_add_point(AlModelPath *path, int index, AlModelPoint point);
AlError al_model_path_remove_point(AlModelPath *path, int index);

bool al_model_path_hit_test(AlModelPath *path, Vec2 point);

#endif
