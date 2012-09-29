/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef WIDGET_GRAPHICS_H
#define WIDGET_GRAPHICS_H

#include "albase/common.h"
#include "alice/widget.h"

AlError widget_graphics_system_init(void);
void widget_graphics_system_free(void);
Vec2 widget_graphics_screen_size(void);

void widget_graphics_render(AlWidget *root, bool renderCursor, Vec2 cursorLocation);

#endif
