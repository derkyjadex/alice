/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/stream.h"

void al_stream_free(AlStream *stream)
{
	if (stream && stream->free) {
		stream->free(stream);
	}
}

AlError al_stream_read_to_string(AlStream *stream, char **result, size_t *resultSize)
{
	BEGIN()

	long size;
	TRY(stream->seek(stream, 0, AL_SEEK_END));
	TRY(stream->tell(stream, &size));
	TRY(stream->seek(stream, 0, AL_SEEK_SET));

	char *string;
	TRY(al_malloc(&string, sizeof(char), size + 1));
	TRY(stream->read(stream, &string, size, NULL));
	string[size] = '\0';

	*result = string;

	if (resultSize) {
		*resultSize = size;
	}

	PASS()
}
