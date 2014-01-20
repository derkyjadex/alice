/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALICE_WIDGET_H__
#define __ALICE_WIDGET_H__

#include <stdbool.h>
#include <SDL2/SDL_keycode.h>

#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/vars.h"
#include "albase/model.h"
#include "albase/lua.h"
#include "host.h"

typedef struct AlWidget AlWidget;
struct AlHost;

AlError al_widget_systems_init(struct AlHost *host, lua_State *L, AlVars *vars);
void al_widget_systems_free(void);

AlError al_widget_init(AlWidget **widget);
void al_widget_free(AlWidget *widget);
void al_widget_add_child(AlWidget *widget, AlWidget *child);
void al_widget_add_sibling(AlWidget *widget, AlWidget *sibling);
void al_widget_remove(AlWidget *widget);
void al_widget_invalidate(AlWidget *widget);

AlError al_widget_send_down(AlWidget *widget, Vec2 location);
AlError al_widget_send_up(AlWidget *widget, Vec2 location);
AlError al_widget_send_motion(AlWidget *widget, Vec2 motion);
AlError al_widget_send_key(AlWidget *widget, SDL_Keycode key);
AlError al_widget_send_text(AlWidget *widget, const char *text);
AlError al_widget_send_keyboard_lost(AlWidget *widget);

AlWidget *al_widget_hit_test(AlWidget *widget, Vec2 location, Vec2 *hitLocation);

#endif
