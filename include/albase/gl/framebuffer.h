/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_FRAMEBUFFER_H__
#define __ALBASE_GL_FRAMEBUFFER_H__

#include "albase/common.h"
#include "albase/gl/opengl.h"

typedef struct AlGlFramebuffer {
	GLuint id;
	GLuint colourTex;
} AlGlFramebuffer;

AlError algl_framebuffer_init(AlGlFramebuffer **framebuffer);
void algl_framebuffer_free(AlGlFramebuffer *framebuffer);
AlError algl_framebuffer_resize(AlGlFramebuffer *framebuffer, int width, int height);

#endif
