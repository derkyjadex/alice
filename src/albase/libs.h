/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef ALICE_LIBS_H
#define ALICE_LIBS_H

#include "albase/lua.h"

int luaopen_fs(lua_State *L);
int luaopen_text(lua_State *L);

#endif
