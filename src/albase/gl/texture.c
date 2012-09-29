/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#if defined(__APPLE__)
	#include <SDL_image/SDL_image.h>
#else
	#include <SDL/SDL_image.h>
#endif

#include "albase/gl/texture.h"

AlError al_gl_texture_init(AlGlTexture **result)
{
	BEGIN()

	AlGlTexture *texture = NULL;
	TRY(al_malloc(&texture, sizeof(AlGlTexture), 1));

	texture->id = 0;
	glGenTextures(1, &texture->id);
	if (texture->id == 0) {
		THROW(AL_ERROR_GENERIC)
	}

	*result = texture;

	CATCH(
		al_gl_texture_free(texture);
	)
	FINALLY()
}

void al_gl_texture_free(AlGlTexture *texture)
{
	if (texture != NULL) {
		glDeleteTextures(1, &texture->id);
		free(texture);
	}
}

static AlError texture_load_from_surface(AlGlTexture *texture, SDL_Surface *surface)
{
	BEGIN()

	SDL_PixelFormat format = {
		NULL, 32, 4,
		0, 0, 0, 0,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		0, 8, 16, 24,
		0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF,
#else
		24, 16, 8, 0,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
#endif
		0, 0
	};

	SDL_Surface *converted = SDL_ConvertSurface(surface, &format, SDL_SWSURFACE);
	if (!converted) {
		al_log_error("Error converting image");
		THROW(AL_ERROR_IO)
	}

	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	PASS(
		SDL_FreeSurface(converted);
		glBindTexture(GL_TEXTURE_2D, 0);
	)
}

AlError al_gl_texture_load_from_file(AlGlTexture *texture, const char *filename)
{
	BEGIN()

	SDL_Surface *surface = IMG_Load(filename);
	if (!surface) {
		al_log_error("Could not open image file: '%s'", filename);
		THROW(AL_ERROR_IO)
	}

	TRY(texture_load_from_surface(texture, surface));

	PASS(
		SDL_FreeSurface(surface);
	)
}

AlError al_gl_texture_load_from_buffer(AlGlTexture *texture, const char *buffer, size_t size)
{
	BEGIN()

	SDL_RWops *ops = NULL;
	SDL_Surface *surface = NULL;

	ops = SDL_RWFromConstMem(buffer, (int)size);
	if (!ops) {
		al_log_error("Could not set up SDL_RWops");
		THROW(AL_ERROR_IO)
	}

	surface = IMG_Load_RW(ops, 1);
	if (!surface) {
		al_log_error("Could not open image data");
		THROW(AL_ERROR_IO)
	}

	TRY(texture_load_from_surface(texture, surface));

	PASS(
		SDL_FreeSurface(surface);
	)
}
