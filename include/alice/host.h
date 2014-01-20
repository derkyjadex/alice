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

typedef struct AlHost AlHost;
struct AlWidget;

AlError al_host_systems_init(void);
void al_host_systems_free(void);

AlError al_host_init(AlHost **host);
void al_host_free(AlHost *host);
AlError al_host_run_script(AlHost *host, const char *filename);

lua_State *al_host_get_lua(AlHost *host);
struct AlWidget *al_host_get_root(AlHost *host);

void al_host_run(AlHost *host);

Vec2 al_host_grab_mouse(AlHost *host, struct AlWidget *widget);
void al_host_release_mouse(AlHost *host, Vec2 location);
AlError al_host_grab_keyboard(AlHost *host, struct AlWidget *widget);
AlError al_host_release_keyboard(AlHost *host);
struct AlWidget *al_host_get_keyboard_widget(AlHost *host);

#endif
