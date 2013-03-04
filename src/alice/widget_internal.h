/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef WIDGET_INTERNAL_H
#define WIDGET_INTERNAL_H

#define FOR_EACH_WIDGET(widget, parent) \
for (AlWidget *widget = parent->firstChild; widget; widget = widget->next)

struct AlWidget {
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
};

#endif
