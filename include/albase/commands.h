/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_COMMAND_H__
#define __ALBASE_COMMAND_H__

#include <stdio.h>

#include "albase/common.h"
#include "albase/lua.h"

AlError al_commands_init(lua_State *L);

AlError al_commands_enqueue(lua_State *L);
AlError al_commands_process_queue(lua_State *L);

#endif
