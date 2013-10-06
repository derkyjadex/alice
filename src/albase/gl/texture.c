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

AlError algl_texture_init(AlGlTexture **result)
{
	BEGIN()

	AlGlTexture *texture = NULL;
	TRY(al_malloc(&texture, sizeof(AlGlTexture)));

	texture->id = 0;
	glGenTextures(1, &texture->id);
	if (texture->id == 0) {
		THROW(AL_ERROR_GENERIC)
	}

	*result = texture;

	CATCH(
		algl_texture_free(texture);
	)
	FINALLY()
}

void algl_texture_free(AlGlTexture *texture)
{
	if (texture != NULL) {
		glDeleteTextures(1, &texture->id);
		al_free(texture);
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

static int rwops_seek(SDL_RWops *rw, int offset, int whence)
{
	AlStream *stream = rw->hidden.unknown.data1;

	AlError error = stream->seek(stream, offset, whence);
	if (error)
		return -1;

	long position;
	stream->tell(stream, &position);

	return (int)position;
}

static int rwops_read(SDL_RWops *rw, void *ptr, int size, int maxnum)
{
	AlStream *stream = rw->hidden.unknown.data1;

	size_t read;
	AlError error = AL_NO_ERROR;

	if (size == 1) {
		error = stream->read(stream, ptr, maxnum, &read);
		if (error)
			return -1;

		return (int)read;

	} else {
		for (int i = 0; i < maxnum; i++) {
			error = stream->read(stream, ptr, size, &read);
			if (error || read != size)
				return -1;

			if (read == 0)
				return i;
		}

		return maxnum;
	}
}

AlError algl_texture_load_from_stream(AlGlTexture *texture, AlStream *stream)
{
	BEGIN()

	SDL_RWops ops = (SDL_RWops){
		.seek = rwops_seek,
		.read = rwops_read,
		.write = NULL,
		.close = NULL,
		.hidden.unknown.data1 = stream
	};

	SDL_Surface *surface = NULL;
	surface = IMG_Load_RW(&ops, 0);
	if (!surface) {
		al_log_error("Could not load image");
		THROW(AL_ERROR_IO);
	}

	TRY(texture_load_from_surface(texture, surface));

	PASS(
		SDL_FreeSurface(surface);
	)
}

AlError algl_texture_load_from_file(AlGlTexture *texture, const char *filename)
{
	BEGIN()

	AlStream *stream = NULL;
	TRY(al_stream_init_filename(&stream, filename, AL_OPEN_READ));
	TRY(algl_texture_load_from_stream(texture, stream));

	PASS(
		al_stream_free(stream);
	)
}

AlError algl_texture_load_from_buffer(AlGlTexture *texture, const char *buffer, size_t size)
{
	BEGIN()

	AlMemStream stream = al_stream_init_mem_stack(buffer, size, "[buffer]");
	TRY(algl_texture_load_from_stream(texture, &stream.base));

	PASS()
}
