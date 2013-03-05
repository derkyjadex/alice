/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef WIDGET_CMDS_H
#define WIDGET_CMDS_H

#include "albase/wrapper.h"
#include "albase/commands.h"

extern AlLuaKey widgetBindings;

AlError al_widget_system_register_commands(AlCommands *commands);
AlError al_widget_system_register_vars(AlVars *vars);
void al_widget_reference(void);

#endif
