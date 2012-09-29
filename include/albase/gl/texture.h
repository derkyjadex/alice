/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_TEXTURE_H__
#define __ALBASE_GL_TEXTURE_H__

#include "albase/common.h"
#include "albase/gl/opengl.h"

typedef struct {
	GLuint id;
} AlGlTexture;

AlError al_gl_texture_init(AlGlTexture **texture);
void al_gl_texture_free(AlGlTexture *texture);
AlError al_gl_texture_load_from_file(AlGlTexture *texture, const char *filename);
AlError al_gl_texture_load_from_buffer(AlGlTexture *texture, const char *buffer, size_t size);

#endif
