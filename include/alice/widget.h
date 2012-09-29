/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALICE_WIDGET_H__
#define __ALICE_WIDGET_H__

#include <stdbool.h>

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

	lua_State *lua;
	AlCommands *commands;
	AlLuaKey downBinding;
	AlLuaKey upBinding;
	AlLuaKey motionBinding;
} AlWidget;

AlError widget_register_commands(AlCommands *commands);
AlError widget_register_vars(AlVars *vars);
AlError widget_init(AlWidget **result, lua_State *lua, AlCommands *commands);
void widget_free(AlWidget *widget);
void widget_add_child(AlWidget *widget, AlWidget *child);
void widget_add_sibling(AlWidget *widget, AlWidget *sibling);
void widget_remove(AlWidget *widget);

AlError widget_send_down(AlWidget *widget);
AlError widget_send_up(AlWidget *widget);
AlError widget_send_motion(AlWidget *widget, Vec2 motion);

AlWidget *widget_hit_test(AlWidget *widget, Vec2 location);

#endif
