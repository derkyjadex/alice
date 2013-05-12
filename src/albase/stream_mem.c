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

static AlError mem_read(AlStream *base, void *ptr, size_t size, size_t *bytesRead)
{
	BEGIN()

	MemStream *stream = (MemStream *)base;

	size_t available = stream->end - stream->cur;

	if (available < size) {
		if (bytesRead) {
			size = available;
		} else {
			THROW(AL_ERROR_IO);
		}
	}

	memcpy(ptr, stream->cur, size);

	stream->cur	+= size;

	if (bytesRead) {
		*bytesRead = size;
	}

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

static AlError mem_tell(AlStream *base, long *offset)
{
	BEGIN()

	MemStream *stream = (MemStream *)base;

	*offset = stream->cur - stream->ptr;

	PASS()
}

static void mem_free(AlStream *base)
{
	MemStream *stream = (MemStream *)base;

	if (stream) {
		free((char *)base->name);

		if (stream->ptr && stream->freePtr) {
			free(stream->ptr);
		}

		free(stream);
	}
}

AlError al_stream_init_mem(AlStream **result, void *ptr, size_t size, bool freePtr, const char *name)
{
	BEGIN()

	MemStream *stream = NULL;
	char *nameCopy = NULL;

	TRY(al_malloc(&stream, sizeof(MemStream), 1));

	if (name) {
		TRY(al_malloc(&nameCopy, sizeof(char), strlen(name) + 1));
		strcpy(nameCopy, name);
	}

	stream->base = (AlStream){
		.name = nameCopy,
		.read = mem_read,
		.write = NULL,
		.seek = mem_seek,
		.tell = mem_tell,
		.free = mem_free
	};

	stream->ptr = ptr;
	stream->cur = ptr;
	stream->end = ptr + size;
	stream->freePtr = freePtr;

	*result = &stream->base;

	CATCH(
		free(nameCopy);
	)
	FINALLY()
}
