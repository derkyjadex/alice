/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_SCRIPT_H__
#define __ALBASE_SCRIPT_H__

#include "albase/common.h"
#include "albase/lua.h"

AlError al_init_lua(lua_State **L);
AlError al_load_base_scripts(lua_State *L);
AlError al_load_scripts(lua_State *L, const char **scripts);

#endif
