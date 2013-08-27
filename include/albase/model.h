/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_MODEL_H__
#define __ALBASE_MODEL_H__

#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/model_shape.h"

struct AlModel;
typedef struct AlModel AlModel;

AlError al_model_use_file(AlModel **model, const char *filename);
AlError al_model_use_shape(AlModel **model, AlModelShape *shape);
AlError al_model_set_shape(AlModel *model, AlModelShape *shape);
void al_model_unuse(AlModel *model);
void al_model_get_bounds(AlModel *model, Box2 *bounds);

#endif
