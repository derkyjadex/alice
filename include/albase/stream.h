/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_STREAM_H__
#define __ALBASE_STREAM_H__

#include <stdio.h>
#include <stdbool.h>

#include "albase/common.h"

typedef enum {
	AL_OPEN_READ,
	AL_OPEN_WRITE
} AlOpenMode;

typedef enum {
	AL_SEEK_SET = SEEK_SET,
	AL_SEEK_CUR = SEEK_CUR,
	AL_SEEK_END = SEEK_END
} AlSeekPos;

typedef struct AlStream AlStream;

struct AlStream {
	AlError (*read)(AlStream *stream, void *ptr, size_t size, size_t *bytesRead);
	AlError (*write)(AlStream *stream, const void *ptr, size_t size);
	AlError (*seek)(AlStream *stream, long offset, AlSeekPos whence);
	AlError (*tell)(AlStream *stream, long *offset);
	void (*free)(AlStream *stream);
};

AlError al_stream_init_file(AlStream **stream, const char *filename, AlOpenMode mode);
AlError al_stream_init_mem(AlStream **stream, void *ptr, size_t size, bool freePtr);

void al_stream_free(AlStream *stream);

#endif
