/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_VARS_H__
#define __ALBASE_VARS_H__

#include <stdbool.h>

#include "albase/common.h"
#include "albase/lua.h"

typedef enum {
	AL_VAR_BOOL,
	AL_VAR_INT,
	AL_VAR_DOUBLE,
	AL_VAR_VEC2,
	AL_VAR_VEC3,
	AL_VAR_VEC4,
	AL_VAR_BOX2,
	AL_VAR_STRING
} AlVarType;

typedef struct AlVars AlVars;

typedef struct {
	const char *name;
	AlVarType type;
	enum { AL_VAR_GLOBAL, AL_VAR_INSTANCE } scope;
	union {
		void *globalPtr;
		size_t instanceOffset;
	} access;
} AlVarReg;

AlError al_vars_init(AlVars **result, lua_State *lua);
void al_vars_free(AlVars *vars);

AlError al_vars_register(AlVars *vars, AlVarReg reg);

#endif
