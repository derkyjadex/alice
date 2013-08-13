/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#if defined(__APPLE__)
	#include <SDL2_image/SDL_image.h>
#else
	#include <SDL2/SDL_image.h>
#endif

#include "albase/gl/texture.h"

AlError algl_texture_init(AlGlTexture **result)
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
		algl_texture_free(texture);
	)
	FINALLY()
}

void algl_texture_free(AlGlTexture *texture)
{
	if (texture != NULL) {
		glDeleteTextures(1, &texture->id);
		free(texture);
	}
}

static AlError texture_load_from_surface(AlGlTexture *texture, SDL_Surface *surface)
{
	BEGIN()

	SDL_PixelFormat *format = NULL;
	SDL_Surface *converted = NULL;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
#else
	format = SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
#endif
	if (!format) {
		al_log_error("Error creating pixel format: %s", SDL_GetError());
		THROW(AL_ERROR_IO)
	}

	converted = SDL_ConvertSurface(surface, format, SDL_SWSURFACE);
	if (!converted) {
		al_log_error("Error converting image: %s", SDL_GetError());
		THROW(AL_ERROR_IO)
	}

	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	PASS(
		SDL_FreeSurface(converted);
		SDL_FreeFormat(format);
		glBindTexture(GL_TEXTURE_2D, 0);
	)
}

static Sint64 rwops_seek(SDL_RWops *rw, Sint64 offset, int whence)
{
	AlStream *stream = rw->hidden.unknown.data1;

	AlError error = stream->seek(stream, offset, whence);
	if (error)
		return 0;

	long position;
	stream->tell(stream, &position);

	return position;
}

static size_t rwops_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum)
{
	AlStream *stream = rw->hidden.unknown.data1;

	size_t read;
	AlError error = AL_NO_ERROR;

	if (size == 1) {
		error = stream->read(stream, ptr, maxnum, &read);
		if (error)
			return 0;

		return read;

	} else {
		for (size_t i = 0; i < maxnum; i++) {
			error = stream->read(stream, ptr, size, &read);
			if (error || read != size)
				return 0;

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

	SDL_Surface *surface = IMG_Load_RW(&ops, 0);
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
	TRY(al_stream_init_file(&stream, filename, AL_OPEN_READ));
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
