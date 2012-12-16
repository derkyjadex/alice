/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef MODEL_EDITING_H
#define MODEL_EDITING_H

#include "albase/common.h"
#include "albase/commands.h"
#include "albase/model_shape.h"


AlModelShape *model_editing_unwrap(lua_State *L);
AlError model_system_init(lua_State *L, AlCommands *commands);

#endif
