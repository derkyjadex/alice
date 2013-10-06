/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_ERROR_H__
#define __ALBASE_ERROR_H__

/**
 * Used to return errors from functions.
 * Non-zero values indicate an error.
 */
typedef enum {
	AL_NO_ERROR = 0,
	AL_ERROR_GENERIC,
	AL_ERROR_MEMORY,
	AL_ERROR_IO,
	AL_ERROR_INVALID_DATA,
	AL_ERROR_GRAPHICS,
	AL_ERROR_SCRIPT,
	AL_ERROR_INVALID_OPERATION
} AlError;

/**
 * Place at the start of a function to set up error handling
 */
#define BEGIN() AlError error = AL_NO_ERROR;

/**
 * Place around a function call that returns an AlError.
 * If the call returns an error, jump to the catch block.
 * @param call The function call to try
 */
#define TRY(call) if ((error = call) != AL_NO_ERROR) { \
	al_log_error("Trace: %s", al_error_to_string(error)); \
	goto catch; \
}

/**
 * Raise an error and jump to the catch block
 * @param err The AlError to return
 */
#define THROW(err) { \
	error = err; \
	al_log_error("Throw: %s", al_error_to_string(error)); \
	goto catch; \
}

/**
 * Immediately jump to the finally block
 */
#define RETURN() goto finally;

/**
 * Runs clean up after errors are thrown by TRY() or THROW().
 * Place at the end of a function, before FINALLY().
 * @param code The clean up code
 */
#define CATCH(code) catch: {\
	if (!error) goto finally; \
	code \
	goto finally; \
}

/**
 * General clean up. Always run before returning.
 * Place at the end of a function, after CATCH().
 * @param code The clean up code
 */
#define FINALLY(code) finally: {\
	code \
	return error; \
}

/**
 * Catch block for Lua functions.
 * The error string is raised as a Lua error.
 * @param code The clean up code
 * @param format The error string format
 * @param ... Values for the error string format
 */
#define CATCH_LUA(code, format, ...) catch: {\
	code \
	goto finally; \
} \
catch_return: \
	return luaL_error(L, format, ##__VA_ARGS__);

/**
 * Finally block for Lua functions.
 * @param code The clean up code
 * @param result Number of values to return from the Lua function
 */
#define FINALLY_LUA(code, result) finally: {\
	code \
	if (error) goto catch_return; \
	return result; \
}

/**
 * Pass error on without CATCH().
 * Always runs clean up code, as with FINALLY(). Place at the end of a function
 * without CATCH() or FINALLY()
 * @param code The clean up code
 */
#define PASS(code) \
	CATCH() \
	FINALLY(code)

/**
 * Get a string for the specified error
 */
const char *al_error_to_string(AlError error);

void _al_log(const char *file, int line, const char *func, const char *format, ...);
void _al_log_error(const char *file, int line, const char *func, const char *format, ...);

/**
 * Writes a message to the log
 * @param format The message string format
 * @param ... Values for the string format
 */
#define al_log(format, ...) _al_log(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Writes a message to the error log
 * @param format The error string format
 * @param ... Values for the error string format
 */
#define al_log_error(format, ...) _al_log_error(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#endif
