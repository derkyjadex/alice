/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef MODEL_SHAPE_CMDS_H
#define MODEL_SHAPE_CMDS_H

#include "albase/common.h"
#include "albase/commands.h"
#include "albase/vars.h"

AlError al_model_commands_init(AlCommands *commands);
AlError al_model_vars_init(AlVars *vars);

#endif
