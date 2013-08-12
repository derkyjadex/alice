/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_OPENGL_H__
#define __ALBASE_GL_OPENGL_H__

#if defined(__APPLE__)
	#define NO_SDL_GLEXT
	#include <SDL2/SDL_opengl.h>
#endif

#if defined(RASPI)
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#endif

#endif
