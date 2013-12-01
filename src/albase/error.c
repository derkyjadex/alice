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
	if (error >= AL_NO_ERROR && (int)error < sizeof(errorStrings) / sizeof(char *)) {
		return errorStrings[error];
	} else {
		return unknownError;
	}
}

static void log(FILE *out, const char *file, int line, const char *func, const char *format, va_list args)
{
	fprintf(out, "%s:%d, %s() ", file, line, func);
	vfprintf(out, format, args);
	fprintf(out, "\n");
}

void _al_log(const char *file, int line, const char *func, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log(stdout, file, line, func, format, args);
	va_end(args);
}

void _al_log_error(const char *file, int line, const char *func, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log(stderr, file, line, func, format, args);
	va_end(args);
}
