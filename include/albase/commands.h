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

typedef struct AlCommands AlCommands;

AlError al_commands_init(AlCommands **commands, lua_State *lua);
void al_commands_free(AlCommands *commands);
AlError al_commands_register(AlCommands *commands, const char *name, lua_CFunction function, ...);
AlError al_commands_enqueue(AlCommands *commands);
AlError al_commands_process_queue(AlCommands *commands);

#endif
