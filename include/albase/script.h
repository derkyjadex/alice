/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_SCRIPT_H__
#define __ALBASE_SCRIPT_H__

#include "albase/common.h"
#include "albase/lua.h"
#include "albase/stream.h"

#define AL_SCRIPT_VAR(name) scripts_##name##_lua
#define AL_SCRIPT_SIZE_VAR(name) scripts_##name##_lua_size
#define AL_SCRIPT_DECLARE(name) \
	extern const char AL_SCRIPT_VAR(name)[]; \
	extern const size_t AL_SCRIPT_SIZE_VAR(name);

#define AL_SCRIPT_RUN(L, name) al_script_run_buffer(L, AL_SCRIPT_VAR(name), AL_SCRIPT_SIZE_VAR(name), #name".lua")

AlError al_script_init(lua_State **L);
AlError al_script_run_base_scripts(lua_State *L);
AlError al_script_run_stream(lua_State *L, AlStream *stream);
AlError al_script_run_file(lua_State *L, const char *filename);
AlError al_script_run_buffer(lua_State *L, const void *ptr, size_t size, const char* name);
void al_script_push_traceback(lua_State *L);
AlError al_script_call(lua_State *L, int nargs, int nresults);

#endif
