/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "albase/error.h"

static const char *errorStrings[] = {
	"no error",
	"generic",
	"memory",
	"io",
	"invalid data",
	"graphics",
	"script",
	"invalid operation"
};

static const char *unknownError = "unknown";

const char *al_error_to_string(AlError error)
{
	if (error >= AL_NO_ERROR && error < sizeof(errorStrings) / sizeof(char *)) {
		return errorStrings[error];
	} else {
		return unknownError;
	}
}

void _al_log_error(const char *file, int line, const char *func, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "%s:%d, %s() ", file, line, func);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	va_end(args);
}
