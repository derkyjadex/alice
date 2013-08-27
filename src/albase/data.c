/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>

#include "albase/data.h"

const uint32_t NO_LENGTH = UINT32_MAX;

#define data_read(data, ptr, size) (data)->stream->read((data)->stream, (ptr), (size), NULL)
#define data_write(data, ptr, size) (data)->stream->write((data)->stream, (ptr), (size))
#define data_seek(data, offset, whence) (data)->stream->seek((data)->stream, (offset), (whence))

struct AlData {
	AlStream *stream;
	void *temp;
};

AlError al_data_init(AlData **result, AlStream *stream)
{
	BEGIN()

	AlData *data = NULL;
	TRY(al_malloc(&data, sizeof(AlData), 1));

	data->stream = stream;
	data->temp = NULL;

	*result = data;

	PASS()
}

void al_data_free(AlData *data)
{
	if (data) {
		free(data->temp);
		free(data);
	}
}

static AlError write_token(AlData *data, AlToken token)
{
	uint8_t t = token;
	return data_write(data, &t, 1);
}

static AlError write_type(AlData *data, AlVarType type)
{
	uint8_t t = type;
	return data_write(data, &t, 1);
}

static AlError read_bool(AlData *data, bool *result)
{
	BEGIN()

	uint8_t value;
	TRY(data_read(data, &value, 1));

	*result = value;

	PASS()
}

static AlError write_bool(AlData *data, const bool *value)
{
	uint8_t val = *value;
	return data_write(data, &val, 1);
}

static AlError read_int(AlData *data, int32_t *result)
{
	return data_read(data, result, 4);
}

static AlError write_int(AlData *data, const int32_t *value)
{
	return data_write(data, value, 4);
}

static AlError read_double(AlData *data, double *result)
{
	return data_read(data, result, 8);
}

static AlError write_double(AlData *data, const double *value)
{
	return data_write(data, value, 8);
}

static AlError read_vec2(AlData *data, Vec2 *result)
{
	return data_read(data, result, 16);
}

static AlError write_vec2(AlData *data, const Vec2 *value)
{
	return data_write(data, value, 16);
}

static AlError read_vec3(AlData *data, Vec3 *result)
{
	return data_read(data, result, 24);
}

static AlError write_vec3(AlData *data, const Vec3 *value)
{
	return data_write(data, value, 24);
}

static AlError read_vec4(AlData *data, Vec4 *result)
{
	return data_read(data, result, 32);
}

static AlError write_vec4(AlData *data, const Vec4 *value)
{
	return data_write(data, value, 32);
}

static AlError read_box2(AlData *data, Box2 *result)
{
	return data_read(data, result, 32);
}

static AlError write_box2(AlData *data, const Box2 *value)
{
	return data_write(data, value, 32);
}

static AlError read_string(AlData *data, char **result, uint32_t *resultLength)
{
	BEGIN()

	uint32_t length;
	char *chars;
	TRY(data_read(data, &length, 4));
	TRY(al_malloc(&chars, sizeof(char), length + 1));
	TRY(data_read(data, chars, length));
	chars[length] = '\0';

	if (data->temp) {
		free(data->temp);
	}

	*result = data->temp = chars;
	if (resultLength) {
		*resultLength = length;
	}

	CATCH(
		free(chars);
	)
	FINALLY()
}

static AlError write_string(AlData *data, const char *value, uint32_t length)
{
	BEGIN()

	uint32_t len;
	if (length != NO_LENGTH) {
		len = length;
	} else {
		len = (uint32_t)strlen(value);
	}

	TRY(data_write(data, &len, 4));
	TRY(data_write(data, value, len));

	PASS()
}

static size_t get_var_size(AlVarType type)
{
	switch (type) {
		case AL_VAR_BOOL: return sizeof(bool);
		case AL_VAR_INT: return sizeof(int32_t);
		case AL_VAR_DOUBLE: return sizeof(double);
		case AL_VAR_VEC2: return sizeof(Vec2);
		case AL_VAR_VEC3: return sizeof(Vec3);
		case AL_VAR_VEC4: return sizeof(Vec4);
		case AL_VAR_BOX2: return sizeof(Box2);
		case AL_VAR_STRING: return sizeof(char *);
		default: return 0;
	}
}

static AlError read_array(AlData *data, AlVarType type, void *result, uint32_t *resultCount)
{
	BEGIN()

	size_t itemSize = get_var_size(type);
	uint32_t length;
	void *array = NULL;
	TRY(data_read(data, &length, 4));
	TRY(al_malloc(&array, itemSize, length));

	for (int i = 0; i < length; i++) {
		void *item = array + (i * itemSize);

		switch (type) {
			case AL_VAR_BOOL: TRY(read_bool(data, item)); break;
			case AL_VAR_INT: TRY(read_int(data, item)); break;
			case AL_VAR_DOUBLE: TRY(read_double(data, item)); break;
			case AL_VAR_VEC2: TRY(read_vec2(data, item)); break;
			case AL_VAR_VEC3: TRY(read_vec3(data, item)); break;
			case AL_VAR_VEC4: TRY(read_vec4(data, item)); break;
			case AL_VAR_BOX2: TRY(read_box2(data, item)); break;
			default:
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	if (data->temp) {
		free(data->temp);
	}

	*(void **)result = data->temp = array;
	*resultCount = length;

	CATCH(
		free(array);
	)
	FINALLY()
}

static AlError write_array(AlData *data, AlVarType type, const void *values, uint32_t count)
{
	BEGIN()

	size_t itemSize = get_var_size(type);

	TRY(data_write(data, &count, 4));

	for (int i = 0; i < count; i++) {
		const void *value = values + (i * itemSize);

		switch (type) {
			case AL_VAR_BOOL: TRY(write_bool(data, value)); break;
			case AL_VAR_INT: TRY(write_int(data, value)); break;
			case AL_VAR_DOUBLE: TRY(write_double(data, value)); break;
			case AL_VAR_VEC2: TRY(write_vec2(data, value)); break;
			case AL_VAR_VEC3: TRY(write_vec3(data, value)); break;
			case AL_VAR_VEC4: TRY(write_vec4(data, value)); break;
			case AL_VAR_BOX2: TRY(write_box2(data, value)); break;
			default:
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	PASS()
}

AlError al_data_read(AlData *data, AlDataItem *item)
{
	BEGIN()

	uint8_t type;
	TRY(data_read(data, &type, 1));

	item->array = false;

	switch (type) {
		case AL_TOKEN_START:
		case AL_TOKEN_END:
			break;

		case AL_TOKEN_TAG: TRY(read_int(data, &item->value.tag)); break;
		case AL_VAR_BOOL: TRY(read_bool(data, &item->value.boolVal)); break;
		case AL_VAR_INT: TRY(read_int(data, &item->value.intVal)); break;
		case AL_VAR_DOUBLE: TRY(read_double(data, &item->value.doubleVal)); break;
		case AL_VAR_VEC2: TRY(read_vec2(data, &item->value.vec2)); break;
		case AL_VAR_VEC3: TRY(read_vec3(data, &item->value.vec3)); break;
		case AL_VAR_VEC4: TRY(read_vec4(data, &item->value.vec4)); break;
		case AL_VAR_BOX2: TRY(read_box2(data, &item->value.box2)); break;
		case AL_VAR_STRING:
			TRY(read_string(data, &item->value.string.chars, &item->value.string.length));
			break;

		case AL_VAR_BOOL | 0x80:
		case AL_VAR_INT | 0x80:
		case AL_VAR_DOUBLE | 0x80:
		case AL_VAR_VEC2 | 0x80:
		case AL_VAR_VEC3 | 0x80:
		case AL_VAR_VEC4 | 0x80:
		case AL_VAR_BOX2 | 0x80:
			type &= 0x7F;
			item->array = true;
			TRY(read_array(data, type, &item->value.array.items, &item->value.array.length));
			break;

		default:
			THROW(AL_ERROR_INVALID_DATA);
	}

	item->type = type;

	PASS()
}

AlError al_data_read_start(AlData *data)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));

	if (item.type != AL_TOKEN_START)
		THROW(AL_ERROR_INVALID_DATA);

	PASS()
}

AlError al_data_read_start_tag(AlData *data, AlDataTag expected, AlDataTag *actual)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));

	switch (item.type) {
		case AL_TOKEN_START:
			TRY(al_data_read(data, &item));
			if (item.type != AL_TOKEN_TAG)
				THROW(AL_ERROR_INVALID_DATA);

			if (expected != AL_ANY_TAG && item.value.tag != expected)
				THROW(AL_ERROR_INVALID_DATA);

			if (actual) {
				*actual = item.value.tag;
			}
			break;

		case AL_TOKEN_END:
			if (expected)
				THROW(AL_ERROR_INVALID_DATA);

			*actual = AL_NO_TAG;
			break;

		default:
			THROW(AL_ERROR_INVALID_DATA);
	}
	
	PASS()
}

AlError al_data_read_value(AlData *data, AlVarType type, void *value)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));
	if (item.type != type || item.array)
		THROW(AL_ERROR_INVALID_DATA);

	switch (type) {
		case AL_VAR_BOOL: *(bool *)value = item.value.boolVal; break;
		case AL_VAR_INT: *(int *)value = item.value.intVal; break;
		case AL_VAR_DOUBLE: *(double *)value = item.value.doubleVal; break;
		case AL_VAR_VEC2: *(Vec2 *)value = item.value.vec2; break;
		case AL_VAR_VEC3: *(Vec3 *)value = item.value.vec3; break;
		case AL_VAR_VEC4: *(Vec4 *)value = item.value.vec4; break;
		case AL_VAR_BOX2: *(Box2 *)value = item.value.box2; break;
		case AL_VAR_STRING: *(const char **)value = item.value.string.chars; break;
	}

	PASS()
}

AlError al_data_read_array(AlData *data, AlVarType type, void *values, uint32_t *count)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));
	if (item.type != type || !item.array)
		THROW(AL_ERROR_INVALID_DATA);

	*(void **)values = item.value.array.items;
	*count = item.value.array.length;
	data->temp = NULL;

	PASS()
}

AlError al_data_skip_rest(AlData *data)
{
	BEGIN()

	bool atEnd = false;
	do {
		uint8_t type;
		TRY(data_read(data, &type, 1));

		switch (type) {
			case AL_TOKEN_START:
				al_data_skip_rest(data);
				break;

			case AL_TOKEN_END:
				atEnd = true;
				break;

			case AL_TOKEN_TAG: TRY(data_seek(data, 4, AL_SEEK_CUR)); break;
			case AL_VAR_BOOL: TRY(data_seek(data, 1, AL_SEEK_CUR)); break;
			case AL_VAR_INT: TRY(data_seek(data, 4, AL_SEEK_CUR)); break;
			case AL_VAR_DOUBLE: TRY(data_seek(data, 8, AL_SEEK_CUR)); break;
			case AL_VAR_VEC2: TRY(data_seek(data, 16, AL_SEEK_CUR)); break;
			case AL_VAR_VEC3: TRY(data_seek(data, 24, AL_SEEK_CUR)); break;
			case AL_VAR_VEC4: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;
			case AL_VAR_BOX2: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;

			case AL_VAR_STRING:
			case AL_VAR_BOOL | 0x80:
			case AL_VAR_INT | 0x80:
			case AL_VAR_DOUBLE | 0x80:
			case AL_VAR_VEC2 | 0x80:
			case AL_VAR_VEC3 | 0x80:
			case AL_VAR_VEC4 | 0x80:
			case AL_VAR_BOX2 | 0x80:
			{
				uint32_t length;
				TRY(data_read(data, &length, 4));
				TRY(data_seek(data, length, AL_SEEK_CUR));
			}
				break;

			default:
				THROW(AL_ERROR_INVALID_DATA);
		}
	} while (!atEnd);

	PASS()
}

AlError al_data_write_start(AlData *data)
{
	return write_token(data, AL_TOKEN_START);
}

AlError al_data_write_end(AlData *data)
{
	return write_token(data, AL_TOKEN_END);
}

AlError al_data_write_start_tag(AlData *data, AlDataTag tag)
{
	BEGIN()

	TRY(al_data_write_start(data));
	TRY(write_token(data, AL_TOKEN_TAG));
	TRY(write_int(data, &tag));

	PASS()
}

AlError al_data_write_simple_tag(AlData *data, AlDataTag tag, AlVarType type, const void *value)
{
	BEGIN()

	TRY(al_data_write_start_tag(data, tag));
	TRY(al_data_write_value(data, type, value));
	TRY(al_data_write_end(data));

	PASS()
}

AlError al_data_write_value(AlData *data, AlVarType type, const void *value)
{
	BEGIN()

	TRY(write_type(data, type));

	switch (type) {
		case AL_VAR_BOOL: TRY(write_bool(data, value)); break;
		case AL_VAR_INT: TRY(write_int(data, value)); break;
		case AL_VAR_DOUBLE: TRY(write_double(data, value)); break;
		case AL_VAR_VEC2: TRY(write_vec2(data, value)); break;
		case AL_VAR_VEC3: TRY(write_vec3(data, value)); break;
		case AL_VAR_VEC4: TRY(write_vec4(data, value)); break;
		case AL_VAR_BOX2: TRY(write_box2(data, value)); break;
		case AL_VAR_STRING: TRY(write_string(data, value, NO_LENGTH)); break;
	}

	PASS()
}

AlError al_data_write_string(AlData *data, const char *value, uint32_t length)
{
	BEGIN()

	TRY(write_type(data, AL_VAR_STRING));
	TRY(write_string(data, value, length));

	PASS()
}

AlError al_data_write_array(AlData *data, AlVarType type, const void *values, uint32_t count)
{
	BEGIN()

	TRY(write_type(data, type | 0x80));
	TRY(write_array(data, type, values, count));

	PASS()
}
