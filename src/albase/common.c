/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "albase/common.h"

AlError al_malloc(void *ptr, size_t size, size_t count)
{
	BEGIN()

	void *result;

	if (size == 0 || count == 0) {
		result = NULL;

	} else {
		result = malloc(size * count);
		if (result == NULL) {
			THROW(AL_ERROR_MEMORY)
		}
	}

	*(void**)ptr = result;

	PASS()
}

AlError al_realloc(void *ptr, size_t size, size_t count)
{
	BEGIN()

	void *result = NULL;

	if (size == 0 || count == 0) {
		free(*(void**)ptr);

	} else {
		result = realloc(*(void**)ptr, size * count);
		if (result == NULL) {
			THROW(AL_ERROR_MEMORY)
		}
	}

	*(void**)ptr = result;

	PASS()
}
