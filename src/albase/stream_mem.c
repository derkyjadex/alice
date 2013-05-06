/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "albase/stream.h"

typedef struct {
	AlStream base;
	void *ptr;
	void *cur;
	void *end;
	bool freePtr;
} MemStream;

static AlError mem_read(AlStream *base, void *ptr, size_t size)
{
	BEGIN()

	MemStream *stream = (MemStream *)base;

	if (stream->cur + size > stream->end)
		THROW(AL_ERROR_IO);

	memcpy(ptr, stream->cur, size);

	stream->cur	+= size;

	PASS()
}

static AlError mem_seek(AlStream *base, long offset, AlSeekPos whence)
{
	BEGIN()

	MemStream *stream = (MemStream *)base;

	void *cur = stream->cur;

	switch (whence) {
		case AL_SEEK_SET:
			cur = stream->ptr + offset;
			break;

		case AL_SEEK_CUR:
			cur = stream->cur + offset;
			break;

		case AL_SEEK_END:
			cur = stream->end + offset;
			break;
	}

	if (cur < stream->ptr || cur > stream->end)
		THROW(AL_ERROR_IO);

	stream->cur = cur;

	PASS()
}

static void mem_free(AlStream *base)
{
	MemStream *stream = (MemStream *)base;

	if (stream) {
		if (stream->ptr && stream->freePtr) {
			free(stream->ptr);
		}

		free(stream);
	}
}

AlError al_stream_init_mem(AlStream **result, void *ptr, size_t size, bool freePtr)
{
	BEGIN()

	MemStream *stream = NULL;

	TRY(al_malloc(&stream, sizeof(MemStream), 1));

	stream->base = (AlStream){
		.read = mem_read,
		.write = NULL,
		.seek = mem_seek,
		.free = mem_free
	};

	stream->ptr = ptr;
	stream->cur = ptr;
	stream->end = ptr + size;
	stream->freePtr = freePtr;

	*result = &stream->base;

	PASS()
}
