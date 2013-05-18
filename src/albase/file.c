/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/file.h"
#include "albase/stream.h"

AlError al_read_file_to_string(const char *filename, char **string)
{
	BEGIN()

	AlStream *stream = NULL;

	TRY(al_stream_init_file(&stream, filename, AL_OPEN_READ));
	TRY(al_stream_read_to_string(stream, string, NULL));

	PASS(
		al_stream_free(stream);
	)
}
