/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "albase/common.h"
#include "albase/lua.h"
#include "libs.h"

static bool is_start_byte(uint8_t b)
{
	return b <= 0x7F || (b >= 0xC2 && b <= 0xF4);
}

static size_t count_chars(const char *text, size_t length)
{
	size_t numChars = 0;

	for (int i = 0; i < length; i++) {
		if (is_start_byte(text[i])) {
			numChars++;
		}
	}

	return numChars;
}

static int cmd_text_length(lua_State *L)
{
	size_t length;
	const char *text = luaL_checklstring(L, 1, &length);

	lua_pushinteger(L, count_chars(text, length));

	return 1;
}

static int cmd_text_insert(lua_State *L)
{
	size_t selfLength, textLength;
	const char *self = luaL_checklstring(L, 1, &selfLength);
	const char *text = luaL_checklstring(L, 2, &textLength);
	size_t position = luaL_checknumber(L, 3) - 1;

	char *result = NULL;
	size_t resultLength = selfLength + textLength;
	size_t insertedChars = 0;

	if (al_malloc(&result, sizeof(char), resultLength) != AL_NO_ERROR)
		return luaL_error(L, "text_insert: memory error");

	char *resultCursor = result;
	const char *selfEnd = self + selfLength;
	const char *textEnd = text + textLength;
	size_t initialChars = 0;

	while (self < selfEnd) {
		char b = *self++;
		if (is_start_byte(b)) {
			initialChars++;
			if (initialChars > position) {
				self--;
				break;
			}
		}

		*resultCursor++ = b;
	}

	while (text < textEnd) {
		char b = *text++;
		*resultCursor++ = b;
		if (is_start_byte(b)) {
			insertedChars++;
		}
	}

	while (self < selfEnd) {
		*resultCursor++ = *self++;
	}

	lua_pushlstring(L, result, resultLength);
	free(result);

	lua_pushnumber(L, insertedChars);

	return 2;
}

static int cmd_text_remove(lua_State *L)
{
	size_t selfLength;
	const char *self = luaL_checklstring(L, 1, &selfLength);
	size_t start = luaL_checknumber(L, 2);
	size_t length = luaL_checknumber(L, 3) + 1;

	char *result = NULL;
	size_t resultLength = selfLength - length + 1;

	if (al_malloc(&result, sizeof(char), resultLength))
		return luaL_error(L, "text_remove: memory error");

	char *resultCursor = result;
	const char *selfEnd = self + selfLength;
	resultLength = 0;

	while (self < selfEnd) {
		char b = *self++;
		if (is_start_byte(b)) {
			start--;
			if (start == 0) {
				self--;
				break;
			}
		}

		*resultCursor++ = b;
		resultLength++;
	}

	while (self < selfEnd) {
		char b = *self++;
		if (is_start_byte(b)) {
			length--;
			if (length == 0) {
				self--;
				break;
			}
		}
	}

	while (self < selfEnd) {
		*resultCursor++ = *self++;
		resultLength++;
	}

	lua_pushlstring(L, result, resultLength);
	free(result);

	return 1;
}

static const luaL_Reg lib[] = {
	{"length", cmd_text_length},
	{"insert", cmd_text_insert},
	{"remove", cmd_text_remove},
	{NULL, NULL}
};

int luaopen_text(lua_State *L)
{
	luaL_newlib(L, lib);
	return 1;
}
