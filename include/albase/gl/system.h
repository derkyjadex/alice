/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_SYSTEM_H__
#define __ALBASE_GL_SYSTEM_H__

#include "albase/common.h"
#include "albase/geometry.h"

AlError al_gl_system_init(void);
void al_gl_system_free(void);
Vec2 al_gl_system_screen_size(void);
void al_gl_system_swap_buffers(void);

#endif
