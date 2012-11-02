/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_VARS_H__
#define __ALBASE_VARS_H__

#include <stdbool.h>

#include "albase/common.h"
#include "albase/commands.h"
#include "albase/wrapper.h"

typedef enum {
	VAR_BOOL, VAR_INT, VAR_DOUBLE,
	VAR_VEC2, VAR_VEC3, VAR_VEC4,
	VAR_BOX, VAR_STRING
} AlVarType;

typedef struct AlVars AlVars;

AlError al_vars_init(AlVars **result, lua_State *lua, AlCommands *commands);
void al_vars_free(AlVars *vars);
AlError al_vars_register_global(AlVars *vars, const char *name, AlVarType type, void *ptr);
AlError al_vars_register_instance(AlVars *vars, const char *name, AlVarType type, size_t offset, AlWrapper *wrapper);

#endif
