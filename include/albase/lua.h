/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_LUA_H__
#define __ALBASE_LUA_H__

#if defined(__APPLE__)

#include <Lua/lua.h>
#include <Lua/lauxlib.h>
#include <Lua/lualib.h>

#else

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>

#endif

typedef char AlLuaKey;

#endif
