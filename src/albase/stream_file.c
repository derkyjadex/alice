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
	bool closeFile;
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

		if (stream->file && stream->closeFile) {
			fclose(stream->file);
		}

		free(stream);
	}
}

AlError al_stream_init_filename(AlStream **result, const char *filename, AlOpenMode mode)
{
	BEGIN()

	FileStream *stream = NULL;
	TRY(al_malloc(&stream, sizeof(FileStream), 1));

	stream->base = (AlStream){
		.name = NULL,
		.read = file_read,
		.write = file_write,
		.seek = file_seek,
		.tell = file_tell,
		.free = file_free
	};
	stream->file = NULL;
	stream->closeFile = true;

	TRY(al_malloc(&stream->base.name, sizeof(char), strlen(filename) + 1));
	strcpy((char *)stream->base.name, filename);

	stream->file = fopen(filename, (mode == AL_OPEN_READ) ? "rb" : "w");
	if (!stream->file) {
		al_log_error("Could not open file: %s", filename);
		THROW(AL_ERROR_IO);
	}

	*result = &stream->base;

	CATCH(
		file_free(&stream->base);
	)
	FINALLY()
}

AlError al_stream_init_file(AlStream **result, FILE *file, bool closeFile, const char *name)
{
	BEGIN()

	FileStream *stream = NULL;
	TRY(al_malloc(&stream, sizeof(FileStream), 1));

	stream->base = (AlStream){
		.name = NULL,
		.read = file_read,
		.write = file_write,
		.seek = file_seek,
		.tell = file_tell,
		.free = file_free
	};
	stream->file = file;
	stream->closeFile = closeFile;

	TRY(al_malloc(&stream->base.name, sizeof(char), strlen(name) + 1));
	strcpy((char *)stream->base.name, name);

	*result = &stream->base;

	CATCH(
		file_free(&stream->base);
	)
	FINALLY()
}
