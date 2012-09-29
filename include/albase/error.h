/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_ERROR_H__
#define __ALBASE_ERROR_H__

typedef enum {
	AL_NO_ERROR = 0,
	AL_ERROR_GENERIC,
	AL_ERROR_MEMORY,
	AL_ERROR_IO,
	AL_ERROR_INVALID_DATA,
	AL_ERROR_GRAPHICS,
	AL_ERROR_SCRIPT
} AlError;

#define BEGIN() AlError error = AL_NO_ERROR;

#define TRY(x) if ((error = x) != AL_NO_ERROR) { \
	al_log_error("Trace: %s", al_error_to_string(error)); \
	goto catch; \
}

#define THROW(x) { \
	error = x; \
	al_log_error("Throw: %s", al_error_to_string(error)); \
	goto catch; \
}

#define CATCH(x) catch: {\
	if (!error) goto finally; \
	x \
	goto finally; \
}

#define FINALLY(x) finally: {\
	x \
	return error; \
}

#define PASS(x) \
	CATCH() \
	FINALLY(x)

const char *al_error_to_string(AlError error);

void _al_log_error(const char *file, int line, const char *func, const char *format, ...);

#define al_log_error(format, ...) _al_log_error(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#endif
