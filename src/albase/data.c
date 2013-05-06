/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>

#include "albase/data.h"

static AlError read_bool(AlStream *stream, bool *result)
{
	BEGIN()

	uint8_t value;
	TRY(stream->read(stream, &value, 1));

	*result = value;

	PASS()
}

static AlError write_bool(AlStream *stream, bool *value)
{
	uint8_t val = *value;
	return stream->write(stream, &val, 1);
}

static AlError read_int(AlStream *stream, int *result)
{
	BEGIN()

	int32_t value;
	TRY(stream->read(stream, &value, 4));

	*result = value;

	PASS()
}

static AlError write_int(AlStream *stream, int *value)
{
	int32_t	val = *value;
	return stream->write(stream, &val, 4);
}

static AlError read_double(AlStream *stream, double *result)
{
	return stream->read(stream, result, 8);
}

static AlError write_double(AlStream *stream, double *value)
{
	return stream->write(stream, &value, 8);
}

static AlError read_vec2(AlStream *stream, Vec2 *result)
{
	return stream->read(stream, result, 16);
}

static AlError write_vec2(AlStream *stream, Vec2 *value)
{
	return stream->write(stream, value, 16);
}

static AlError read_vec3(AlStream *stream, Vec3 *result)
{
	return stream->read(stream, result, 24);
}

static AlError write_vec3(AlStream *stream, Vec3 *value)
{
	return stream->write(stream, value, 24);
}

static AlError read_vec4(AlStream *stream, Vec4 *result)
{
	return stream->read(stream, result, 32);
}

static AlError write_vec4(AlStream *stream, Vec4 *value)
{
	return stream->write(stream, value, 32);
}

static AlError read_box(AlStream *stream, Box *result)
{
	return stream->read(stream, result, 32);
}

static AlError write_box(AlStream *stream, Box *value)
{
	return stream->write(stream, value, 32);
}

static AlError read_string(AlStream *stream, char **result, int *resultLength)
{
	BEGIN()

	uint32_t length;
	char *chars;
	TRY(stream->read(stream, &length, 4));
	TRY(al_malloc(&chars, sizeof(char), length + 1));
	TRY(stream->read(stream, chars, length));
	chars[length] = '\0';

	*result = chars;
	if (resultLength) {
		*resultLength = length;
	}

	CATCH(
		free(chars);
	)
	FINALLY()
}

static AlError write_string(AlStream *stream, const char *value, int length)
{
	BEGIN()

	uint32_t len;
	if (length > 0) {
		len = length;
	} else {
		len = (uint32_t)strlen(value);
	}

	TRY(stream->write(stream, &len, 4));
	TRY(stream->write(stream, value, len));

	PASS()
}

void al_data_item_init(AlDataItem *item)
{
	item->value.string.chars = NULL;
}

void al_data_item_free(AlDataItem *item)
{
	free((char *)item->value.string.chars);
}

AlError al_data_read(AlStream *stream, AlDataItem *item)
{
	BEGIN()

	uint8_t type;
	TRY(stream->read(stream, &type, 1));

	switch (type) {
		case AL_TOKEN_START:
		case AL_TOKEN_END:
			break;

		case VAR_BOOL: TRY(read_bool(stream, &item->value.boolVal)); break;
		case VAR_INT: TRY(read_int(stream, &item->value.intVal)); break;
		case VAR_DOUBLE: TRY(read_double(stream, &item->value.doubleVal)); break;
		case VAR_VEC2: TRY(read_vec2(stream, &item->value.vec2)); break;
		case VAR_VEC3: TRY(read_vec3(stream, &item->value.vec3)); break;
		case VAR_VEC4: TRY(read_vec4(stream, &item->value.vec4)); break;
		case VAR_BOX: TRY(read_box(stream, &item->value.box)); break;

		case VAR_STRING:
		{
			char **chars = (char **)&item->value.string.chars;
			free(*chars);
			TRY(read_string(stream, chars, &item->value.string.length));
		}
			break;

		default:
			THROW(AL_ERROR_INVALID_DATA);
	}

	item->type = type;

	PASS()
}

static size_t get_var_size(AlVarType type)
{
	switch (type) {
		case VAR_BOOL: return sizeof(bool);
		case VAR_INT: return sizeof(int);
		case VAR_DOUBLE: return sizeof(double);
		case VAR_VEC2: return sizeof(Vec2);
		case VAR_VEC3: return sizeof(Vec3);
		case VAR_VEC4: return sizeof(Vec4);
		case VAR_BOX: return sizeof(Box);
		case VAR_STRING: return sizeof(char *);
		default: return 0;
	}
}

AlError al_data_read_rest(AlStream *stream, AlVarType arrayType, void *result, int *resultCount)
{
	BEGIN()

	size_t itemSize = get_var_size(arrayType);
	int size = (*resultCount) ? *resultCount : 4;
	void *array = NULL;
	int count = 0;

	TRY(al_malloc(&array, itemSize, size));

	while (true) {
		uint8_t type;
		TRY(stream->read(stream, &type, 1));

		if (type == AL_TOKEN_START)
			THROW(AL_ERROR_INVALID_DATA);

		if (type == AL_TOKEN_END)
			break;

		if (type != arrayType)
			THROW(AL_ERROR_INVALID_DATA);

		if (count == size) {
			TRY(al_realloc(&array, itemSize, size * 2));
			size *= 2;
		}

		void *item = array + ((count + 1) * itemSize);

		switch (type) {
			case VAR_BOOL: TRY(read_bool(stream, item)); break;
			case VAR_INT: TRY(read_int(stream, item)); break;
			case VAR_DOUBLE: TRY(read_double(stream, item)); break;
			case VAR_VEC2: TRY(read_vec2(stream, item)); break;
			case VAR_VEC3: TRY(read_vec3(stream, item)); break;
			case VAR_VEC4: TRY(read_vec4(stream, item)); break;
			case VAR_BOX: TRY(read_box(stream, item)); break;
			case VAR_STRING: TRY(read_string(stream, item, NULL)); break;
		}

		count++;
	}

	*(void **)result = array;
	*resultCount = count;

	CATCH(
		if (array) {
		  if (arrayType == VAR_STRING) {
			  for (int i = 0; i < count; i++) {
				  free(((char *)array)[i]);
			  }
		  }

		  free(array);
		}
	)
	FINALLY()
}

AlError al_data_skip_rest(AlStream *stream)
{
	BEGIN()

	bool atEnd = false;

	while (!atEnd) {
		uint8_t type;
		TRY(stream->read(stream, &type, 1));

		switch (type) {
			case AL_TOKEN_START:
				al_data_skip_rest(stream);
				break;

			case AL_TOKEN_END:
				atEnd = true;
				break;

			case VAR_BOOL: TRY(stream->seek(stream, 1, AL_SEEK_CUR)); break;
			case VAR_INT: TRY(stream->seek(stream, 4, AL_SEEK_CUR)); break;
			case VAR_DOUBLE: TRY(stream->seek(stream, 8, AL_SEEK_CUR)); break;
			case VAR_VEC2: TRY(stream->seek(stream, 16, AL_SEEK_CUR)); break;
			case VAR_VEC3: TRY(stream->seek(stream, 24, AL_SEEK_CUR)); break;
			case VAR_VEC4: TRY(stream->seek(stream, 32, AL_SEEK_CUR)); break;
			case VAR_BOX: TRY(stream->seek(stream, 32, AL_SEEK_CUR)); break;

			case VAR_STRING:
			{
				uint32_t length;
				TRY(stream->read(stream, &length, 4));
				TRY(stream->seek(stream, length, AL_SEEK_CUR));
			}
				break;

			default:
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	PASS()
}

AlError al_data_write_start(AlStream *stream)
{
	uint8_t start = AL_TOKEN_START;
	return stream->write(stream, &start, 1);
}

AlError al_data_write_end(AlStream *stream)
{
	uint8_t end = AL_TOKEN_END;
	return stream->write(stream, &end, 1);
}

AlError al_data_write_value(AlStream *stream, AlVarType type, void *value)
{
	BEGIN()

	switch (type) {
		case VAR_BOOL: TRY(write_bool(stream, value)); break;
		case VAR_INT: TRY(write_int(stream, value)); break;
		case VAR_DOUBLE: TRY(write_double(stream, value)); break;
		case VAR_VEC2: TRY(write_vec2(stream, value)); break;
		case VAR_VEC3: TRY(write_vec3(stream, value)); break;
		case VAR_VEC4: TRY(write_vec4(stream, value)); break;
		case VAR_BOX: TRY(write_box(stream, value)); break;
		case VAR_STRING: TRY(write_string(stream, value, -1)); break;
	}

	PASS()
}

AlError al_data_write_values(AlStream *stream, AlVarType type, void *values, int count)
{
	BEGIN()

	size_t itemSize = get_var_size(type);

	for (int i = 0; i < count; i++) {
		TRY(al_data_write_value(stream, type, values + (i * itemSize)));
	}

	PASS()
}

AlError al_data_write_string(AlStream *stream, const char *value, int length)
{
	return write_string(stream, value, length);
}
