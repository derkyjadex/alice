/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef WIDGET_CMDS_H
#define WIDGET_CMDS_H

#include "albase/wrapper.h"

extern AlLuaKey widgetBindings;

int luaopen_widget(lua_State *L);
AlError al_widget_system_register_vars(lua_State *L);

#endif
