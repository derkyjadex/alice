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
	FILE *file;
} FileStream;

static AlError file_read(AlStream *base, void *ptr, size_t size, size_t *bytesRead)
{
	BEGIN()

	FileStream *stream = (FileStream *)base;

	size_t result = fread(ptr, 1, size, stream->file);

	if (result < size && (!feof(stream->file) || !bytesRead))
		THROW(AL_ERROR_IO);

	if (bytesRead) {
		*bytesRead = result;
	}

	PASS()
}

static AlError file_write(AlStream *base, const void *ptr, size_t size)
{
	BEGIN()

	FileStream *stream = (FileStream *)base;

	if (fwrite(ptr, size, 1, stream->file) != 1)
		THROW(AL_ERROR_IO);

	PASS()
}

static AlError file_seek(AlStream *base, long offset, AlSeekPos whence)
{
	BEGIN()

	FileStream *stream = (FileStream *)base;

	if (fseek(stream->file, offset, whence))
		THROW(AL_ERROR_IO);

	PASS()
}

static AlError file_tell(AlStream *base, long *result)
{
	BEGIN()

	FileStream *stream = (FileStream *)base;

	long offset = ftell(stream->file);

	if (offset < 0)
		THROW(AL_ERROR_IO);

	*result = offset;

	PASS()
}

static void file_free(AlStream *base)
{
	FileStream *stream = (FileStream *)base;

	if (stream) {
		free((char *)base->name);

		if (stream->file) {
			fclose(stream->file);
		}

		free(stream);
	}
}

AlError al_stream_init_file(AlStream **result, const char *filename, AlOpenMode mode)
{
	BEGIN()

	FileStream *stream = NULL;
	char *filenameCopy = NULL;

	TRY(al_malloc(&stream, sizeof(FileStream), 1));

	TRY(al_malloc(&filenameCopy, sizeof(char), strlen(filename) + 1));
	strcpy(filenameCopy, filename);

	stream->base = (AlStream){
		.name = filenameCopy,
		.read = file_read,
		.write = file_write,
		.seek = file_seek,
		.tell = file_tell,
		.free = file_free
	};

	stream->file = NULL;
	stream->file = fopen(filename, (mode == AL_OPEN_READ) ? "rb" : "w");
	if (stream->file == NULL) {
		al_log_error("Could not open file: %s", filename);
		THROW(AL_ERROR_IO);
	}

	*result = &stream->base;

	CATCH(
		file_free(&stream->base);
		free(filenameCopy);
	)
	FINALLY()
}
