/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL2/SDL.h>

#include "albase/common.h"
#include "albase/gl/system.h"

static SDL_Window *window = NULL;
static SDL_GLContext context = NULL;

AlError algl_system_init()
{
	BEGIN()

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window = SDL_CreateWindow("Albase", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	if (!window) {
		al_log_error("Error creating window: %s", SDL_GetError());
		THROW(AL_ERROR_GRAPHICS)
	}

	context = SDL_GL_CreateContext(window);
	if (!context) {
		al_log_error("Error creating context: %s", SDL_GetError());
		THROW(AL_ERROR_GRAPHICS)
	}

	SDL_GL_SetSwapInterval(1);

	PASS()
}

void algl_system_free()
{
	SDL_GL_DeleteContext(context);
	context = NULL;

	SDL_DestroyWindow(window);
	window = NULL;
}

Vec2 algl_system_screen_size()
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	return (Vec2){width, height};
}

void algl_system_swap_buffers()
{
	SDL_GL_SwapWindow(window);
}
