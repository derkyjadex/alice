/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALICE_WIDGET_H__
#define __ALICE_WIDGET_H__

#include <stdbool.h>
#include <SDL/SDL_keysym.h>

#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/commands.h"
#include "albase/vars.h"
#include "albase/model.h"
#include "albase/lua.h"

#define FOR_EACH_WIDGET(widget, parent) \
	for (AlWidget *widget = parent->firstChild; widget; widget = widget->next)

typedef struct AlWidget {
	struct AlWidget *next;
	struct AlWidget *prev;
	struct AlWidget *parent;
	struct AlWidget *firstChild;
	struct AlWidget *lastChild;

	bool valid;
	Vec2 location;
	Box bounds;

	Vec4 fillColour;
	struct {
		Vec4 colour;
		int width;
	} border;
	struct {
		Vec2 size;
		Vec2 offset;
		Vec3 colour;
	} grid;
	struct {
		AlModel *model;
		Vec2 location;
		double scale;
	} model;
	struct {
		char *value;
		Vec3 colour;
		double size;
		Vec2 location;
	} text;

	AlLuaKey downBinding;
	AlLuaKey upBinding;
	AlLuaKey motionBinding;
	AlLuaKey keyBinding;
	AlLuaKey textBinding;
	AlLuaKey keyboardLostBinding;
} AlWidget;

AlError al_widget_systems_init(lua_State *L, AlCommands *commands, AlVars *vars);
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
AlError al_widget_send_key(AlWidget *widget, SDLKey key);
AlError al_widget_send_text(AlWidget *widget, const char *text);
AlError al_widget_send_keyboard_lost(AlWidget *widget);

AlWidget *al_widget_hit_test(AlWidget *widget, Vec2 location, Vec2 *hitLocation);

void al_widget_push_userdata(AlWidget *widget);

#endif
