/*
 * Copyright (c) 2014 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL2/SDL_atomic.h>
#include <stdbool.h>

#include "albase/triple_buffer.h"

#define READ_BITS (0x03 << 0)
#define DATA_BITS (0x03 << 2)
#define WRITE_BITS (0x03 << 4)
#define UPDATED_BIT (0x01 << 6)

struct AlTripleBuffer {
	void *pages[3];
	SDL_atomic_t flags;
};

AlError al_triple_buffer_init(AlTripleBuffer **result, size_t size, const void *initial)
{
	BEGIN()

	AlTripleBuffer *buffer = NULL;
	TRY(al_malloc(&buffer, sizeof(AlTripleBuffer)));

	buffer->pages[0] = NULL;
	buffer->pages[1] = NULL;
	buffer->pages[2] = NULL;
	SDL_AtomicSet(&buffer->flags, 0x06);

	TRY(al_malloc(&buffer->pages[0], size));
	TRY(al_malloc(&buffer->pages[1], size));
	TRY(al_malloc(&buffer->pages[2], size));

	memcpy(buffer->pages[0], initial, size);
	memcpy(buffer->pages[1], initial, size);
	memcpy(buffer->pages[2], initial, size);

	*result = buffer;

	CATCH({
		al_triple_buffer_free(buffer);
	})
	FINALLY()
}

void al_triple_buffer_free(AlTripleBuffer *buffer)
{
	if (buffer) {
		al_free(buffer->pages[0]);
		al_free(buffer->pages[1]);
		al_free(buffer->pages[2]);
		al_free(buffer);
	}
}

const void *al_triple_buffer_read(AlTripleBuffer *buffer)
{
	int flags = SDL_AtomicGet(&buffer->flags);

	if (flags & UPDATED_BIT) {
		while (true) {
			int newFlags =
				(flags & WRITE_BITS) |
				((flags & READ_BITS) << 2) |
				((flags & DATA_BITS) >> 2);

			if (SDL_AtomicCAS(&buffer->flags, flags, newFlags)) {
				flags = newFlags;
				break;
			}

			flags = SDL_AtomicGet(&buffer->flags);
		}
	}

	return buffer->pages[flags & READ_BITS];
}

void *al_triple_buffer_write(AlTripleBuffer *buffer)
{
	int flags = SDL_AtomicGet(&buffer->flags);
	return buffer->pages[(flags & WRITE_BITS) >> 4];
}

void al_triple_buffer_flip(AlTripleBuffer *buffer)
{
	int flags, newFlags;
	do {
		flags = SDL_AtomicGet(&buffer->flags);
		newFlags =
			UPDATED_BIT |
			((flags & DATA_BITS) << 2) |
			((flags & WRITE_BITS) >> 2) |
			(flags & READ_BITS);

	} while (!SDL_AtomicCAS(&buffer->flags, flags, newFlags));
}
