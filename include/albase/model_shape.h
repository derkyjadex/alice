/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_MODEL_SHAPE_H__
#define __ALBASE_MODEL_SHAPE_H__

#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/lua.h"
#include "albase/commands.h"

typedef struct {
	Vec3 colour;
	int numPoints;
	size_t pointsLength;
	Vec2 *points;
} AlModelPath;

typedef struct {
	int numPaths;
	size_t pathsLength;
	AlModelPath **paths;
} AlModelShape;

AlError al_model_systems_init(lua_State *L, AlCommands *commands);
void al_model_systems_free(void);

AlError al_model_shape_register_ctor(void);
AlError al_model_path_register_ctor(void);

AlError al_model_shape_init(AlModelShape **shape);
void al_model_shape_free(AlModelShape *shape);

AlError al_model_shape_load(AlModelShape *shape, const char *filename);
AlError al_model_shape_save(AlModelShape *shape, const char *filename);

AlError al_model_shape_add_path(AlModelShape *shape, int index, Vec2 start, Vec2 end);
AlError al_model_shape_remove_path(AlModelShape *shape, int index);

AlError al_model_path_add_point(AlModelPath *path, int index, Vec2 location);
AlError al_model_path_remove_point(AlModelPath *path, int index);

void al_model_shape_push_userdata(AlModelShape *shape);
void al_model_path_push_userdata(AlModelPath *path);

#endif
