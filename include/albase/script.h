/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_SCRIPT_H__
#define __ALBASE_SCRIPT_H__

#include "albase/common.h"
#include "albase/lua.h"

#define AL_SCRIPT_VAR(name) scripts_##name##_lua
#define AL_SCRIPT_SIZE_VAR(name) scripts_##name##_lua_size
#define AL_SCRIPT_DECLARE(name) \
	extern const char AL_SCRIPT_VAR(name)[]; \
	extern const size_t AL_SCRIPT_SIZE_VAR(name);

#define AL_SCRIPT(name) {#name".lua", AL_SCRIPT_VAR(name), AL_SCRIPT_SIZE_VAR(name)}
#define AL_SCRIPT_END {NULL, NULL, 0}

typedef struct {
	const char *name;
	const char *source;
	size_t length;
} AlScript;

AlError al_script_init(lua_State **L);
AlError al_script_run_base_scripts(lua_State *L);
AlError al_script_run_scripts(lua_State *L, const AlScript *scripts);
AlError al_script_run_file(lua_State *L, const char *filename);

#endif
