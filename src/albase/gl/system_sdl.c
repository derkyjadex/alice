/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL/SDL.h>

#include "albase/common.h"
#include "albase/gl/system.h"

static SDL_Surface *screen = NULL;

AlError al_gl_system_init()
{
	BEGIN()

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	Uint32 flags = SDL_OPENGL;

	screen = SDL_SetVideoMode(800, 600, 0, flags);
	if (screen == NULL) {
		THROW(AL_ERROR_GRAPHICS)
	}

	PASS()
}

void al_gl_system_free()
{
	screen = NULL;
}

Vec2 al_gl_system_screen_size()
{
	return (Vec2){screen->w, screen->h};
}

void al_gl_system_swap_buffers()
{
	SDL_GL_SwapBuffers();
}
