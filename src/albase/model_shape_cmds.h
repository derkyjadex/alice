/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef MODEL_SHAPE_CMDS_H
#define MODEL_SHAPE_CMDS_H

#include "albase/common.h"
#include "albase/vars.h"

int luaopen_model(lua_State *L);
AlError al_model_vars_init(lua_State *L);

#endif
