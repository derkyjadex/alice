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
	AlMemStream base;
	bool freePtr;
} HeapMemStream;

static AlError mem_read(AlStream *base, void *ptr, size_t size, size_t *bytesRead)
{
	BEGIN()

	AlMemStream *stream = (AlMemStream *)base;

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

	AlMemStream *stream = (AlMemStream *)base;

	const void *cur = stream->cur;

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

	AlMemStream *stream = (AlMemStream *)base;

	*offset = stream->cur - stream->ptr;

	PASS()
}

static void mem_free(AlStream *base)
{
	HeapMemStream *stream = (HeapMemStream *)base;

	if (stream) {
		al_free((char *)base->name);

		if (stream->base.ptr && stream->freePtr) {
			al_free((void *)stream->base.ptr);
		}

		al_free(stream);
	}
}

AlError al_stream_init_mem(AlStream **result, void *ptr, size_t size, bool freePtr, const char *name)
{
	BEGIN()

	HeapMemStream *stream = NULL;
	char *nameCopy = NULL;

	TRY(al_malloc(&stream, sizeof(HeapMemStream)));

	if (name) {
		TRY(al_malloc(&nameCopy, strlen(name) + 1));
		strcpy(nameCopy, name);
	}

	stream->base = (AlMemStream){
		.base = {
			.name = nameCopy,
			.read = mem_read,
			.write = NULL,
			.seek = mem_seek,
			.tell = mem_tell,
			.free = mem_free
		},
		.ptr = ptr,
		.cur = ptr,
		.end = ptr + size
	};

	stream->freePtr = freePtr;

	*result = &stream->base.base;

	CATCH({
		al_free(nameCopy);
	})
	FINALLY()
}

AlMemStream al_stream_init_mem_stack(const void *ptr, size_t size, const char *name)
{
	return (AlMemStream){
		.base = {
			.name = name,
			.read = mem_read,
			.write = NULL,
			.seek = mem_seek,
			.tell = mem_tell,
			.free = NULL
		},
		.ptr = ptr,
		.cur = ptr,
		.end = ptr + size
	};
}
