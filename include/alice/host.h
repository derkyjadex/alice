/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALICE_HOST_H__
#define __ALICE_HOST_H__

#include <stdbool.h>

#include "albase/common.h"
#include "albase/commands.h"
#include "albase/vars.h"
#include "albase/lua.h"

#include "widget.h"

typedef struct {
	lua_State *lua;
	AlCommands *commands;
	AlVars *vars;
	bool finished;

	Vec2 screenSize;

	AlWidget *root;
	AlWidget *grabbingWidget;
} AlHost;

AlError al_host_systems_init(void);
void al_host_systems_free(void);

AlError al_host_init(AlHost **host);
void al_host_free(AlHost *host);
AlError al_host_run_script(AlHost *host, const char *filename);

void al_host_run(AlHost *host);

#endif
