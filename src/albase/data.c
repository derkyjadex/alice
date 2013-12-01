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
	bool eof;
	void *temp;
};

AlError al_data_init(AlData **result, AlStream *stream)
{
	BEGIN()

	AlData *data = NULL;
	TRY(al_malloc(&data, sizeof(AlData)));

	data->stream = stream;
	data->eof = false;
	data->temp = NULL;

	*result = data;

	PASS()
}

void al_data_free(AlData *data)
{
	if (data) {
		al_free(data->temp);
		al_free(data);
	}
}

static AlError read_uint(AlData *data, uint64_t *result)
{
	BEGIN()

	uint8_t byte;
	int length = 0;

	*result = 0;
	do {
		if (length > 10) {
			al_log_error("varint too long");
			THROW(AL_ERROR_INVALID_DATA);
		}

		TRY(data_read(data, &byte, 1));

		*result |= (byte & 0x7F) << (length * 7);
		length++;
	} while (byte & 0x80);

	PASS()
}

static AlError write_uint(AlData *data, uint64_t value)
{
	uint8_t buffer[10];
	int length = 0;

	do {
		uint8_t byte = value & 0x7F;
		value >>= 7;

		if (value) {
			byte |= 0x80;
		}

		buffer[length++] = byte;
	} while (value);

	return data_write(data, buffer, length);
}

static AlError skip_uint(AlData *data)
{
	BEGIN()

	uint8_t byte;
	do {
		TRY(data_read(data, &byte, 1));
	} while (byte & 0x80);

	PASS()
}

static AlError read_sint(AlData *data, int64_t *result)
{
	BEGIN()

	uint64_t uvalue;
	TRY(read_uint(data, &uvalue));

	*result = (uvalue & 1) ?
	~(uvalue >> 1) : (uvalue >> 1);

	PASS()
}

static AlError write_sint(AlData *data, int64_t value)
{
	uint64_t uvalue = (value < 0) ?
		~(value << 1) : value << 1;

	return write_uint(data, uvalue);
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

static AlError read_tag(AlData *data, AlDataTag *tag)
{
	return data_read(data, tag, 4);
}

static AlError write_tag(AlData *data, const AlDataTag *result)
{
	return data_write(data, result, 4);
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
	BEGIN()

	int64_t value;
	TRY(read_sint(data, &value));

	if (value > INT32_MAX || value < INT32_MIN) {
		al_log_error("value out of range for int32");
		THROW(AL_ERROR_INVALID_DATA);
	}

	*result = (int32_t)value;

	PASS()
}

static AlError write_int(AlData *data, const int32_t *value)
{
	return write_sint(data, *value);
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

static AlError read_string(AlData *data, char **result, uint64_t *resultLength)
{
	BEGIN()

	uint64_t length;
	char *chars = NULL;
	TRY(read_uint(data, &length));

	if (length > SIZE_MAX) {
		al_log_error("string too long to fit in memory");
		THROW(AL_ERROR_MEMORY);
	}

	TRY(al_malloc(&chars, length + 1));
	TRY(data_read(data, chars, length));
	chars[length] = '\0';

	if (data->temp) {
		al_free(data->temp);
	}

	*result = data->temp = chars;
	if (resultLength) {
		*resultLength = length;
	}

	CATCH({
		al_free(chars);
	})
	FINALLY()
}

static AlError write_string(AlData *data, const char *value, uint64_t length)
{
	BEGIN()

	if (length == NO_LENGTH) {
		length = strlen(value);
	}

	TRY(write_uint(data, length));
	TRY(data_write(data, value, length));

	PASS()
}

static AlError skip_string(AlData *data)
{
	BEGIN()

	uint64_t length;
	TRY(read_uint(data, &length));
	TRY(data_seek(data, length, AL_SEEK_CUR));

	PASS()
}

static AlError read_blob(AlData *data, AlBlob *result)
{
	BEGIN()

	uint64_t length;
	uint8_t	*bytes = NULL;
	TRY(read_uint(data, &length));

	if (length > SIZE_MAX) {
		al_log_error("blob too large to fit in memory");
		THROW(AL_ERROR_MEMORY);
	}

	TRY(al_malloc(&bytes, length));
	TRY(data_read(data, bytes, length));

	if (data->temp) {
		al_free(data->temp);
	}

	data->temp = bytes;

	*result = (AlBlob){
		.length = length,
		.bytes = bytes
	};

	CATCH({
		al_free(bytes);
	})
	FINALLY()
}

static AlError write_blob(AlData *data, const AlBlob *value)
{
	BEGIN()

	TRY(write_uint(data, value->length));
	TRY(data_write(data, value->bytes, value->length));

	PASS()
}

static AlError skip_blob(AlData *data)
{
	return skip_string(data);
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
		case AL_VAR_BLOB: return sizeof(AlBlob);
		default: return 0;
	}
}

static AlError read_array(AlData *data, AlVarType type, void *result, uint64_t *resultCount)
{
	BEGIN()

	size_t itemSize = get_var_size(type);
	uint64_t count;
	void *array = NULL;
	TRY(read_uint(data, &count));
	TRY(al_malloc(&array, itemSize * count));

	for (uint64_t i = 0; i < count; i++) {
		void *item = array + (i * itemSize);

		switch (type) {
			case AL_VAR_BOOL: TRY(read_bool(data, item)); break;
			case AL_VAR_INT: TRY(read_int(data, item)); break;
			case AL_VAR_DOUBLE: TRY(read_double(data, item)); break;
			case AL_VAR_VEC2: TRY(read_vec2(data, item)); break;
			case AL_VAR_VEC3: TRY(read_vec3(data, item)); break;
			case AL_VAR_VEC4: TRY(read_vec4(data, item)); break;
			case AL_VAR_BOX2: TRY(read_box2(data, item)); break;

			case AL_VAR_STRING:
				al_log_error("arrays of strings not supported");
				THROW(AL_ERROR_INVALID_DATA);
			case AL_VAR_BLOB:
				al_log_error("arrays of blobs not supported");
				THROW(AL_ERROR_INVALID_DATA);
			default:
				al_log_error("unknown value type: 0x%02x", type);
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	if (data->temp) {
		al_free(data->temp);
	}

	*(void **)result = data->temp = array;
	*resultCount = count;

	CATCH({
		al_free(array);
	})
	FINALLY()
}

static AlError write_array(AlData *data, AlVarType type, const void *values, uint64_t count)
{
	BEGIN()

	size_t itemSize = get_var_size(type);

	TRY(write_uint(data, count));

	for (uint64_t i = 0; i < count; i++) {
		const void *value = values + (i * itemSize);

		switch (type) {
			case AL_VAR_BOOL: TRY(write_bool(data, value)); break;
			case AL_VAR_INT: TRY(write_int(data, value)); break;
			case AL_VAR_DOUBLE: TRY(write_double(data, value)); break;
			case AL_VAR_VEC2: TRY(write_vec2(data, value)); break;
			case AL_VAR_VEC3: TRY(write_vec3(data, value)); break;
			case AL_VAR_VEC4: TRY(write_vec4(data, value)); break;
			case AL_VAR_BOX2: TRY(write_box2(data, value)); break;

			case AL_VAR_STRING:
				al_log_error("arrays of strings not supported");
				THROW(AL_ERROR_INVALID_OPERATION);
			case AL_VAR_BLOB:
				al_log_error("arrays of blobs not supported");
				THROW(AL_ERROR_INVALID_OPERATION);
			default:
				al_log_error("unknown value type: 0x%02x", type);
				THROW(AL_ERROR_INVALID_OPERATION);
		}
	}

	PASS()
}

static AlError skip_array(AlData *data, AlVarType type)
{
	BEGIN()

	uint64_t count;
	TRY(read_uint(data, &count));

	for (uint64_t i = 0; i < count; i++) {
		switch (type) {
			case AL_VAR_BOOL: TRY(data_seek(data, 1, AL_SEEK_CUR)); break;
			case AL_VAR_INT: TRY(skip_uint(data)); break;
			case AL_VAR_DOUBLE: TRY(data_seek(data, 8, AL_SEEK_CUR)); break;
			case AL_VAR_VEC2: TRY(data_seek(data, 16, AL_SEEK_CUR)); break;
			case AL_VAR_VEC3: TRY(data_seek(data, 24, AL_SEEK_CUR)); break;
			case AL_VAR_VEC4: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;
			case AL_VAR_BOX2: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;

			case AL_VAR_STRING:
				al_log_error("arrays of strings not supported");
				THROW(AL_ERROR_INVALID_DATA);
			case AL_VAR_BLOB:
				al_log_error("arrays of blobs not supported");
				THROW(AL_ERROR_INVALID_DATA);
			default:
				al_log_error("unknown value type: 0x%02x", type);
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	PASS()
}

AlError al_data_read(AlData *data, AlDataItem *item)
{
	BEGIN()

	if (data->eof) {
		al_log_error("unexpected end of stream");
		THROW(AL_ERROR_INVALID_DATA);
	}

	uint8_t type;
	size_t bytesRead;
	data->stream->read(data->stream, &type, 1, &bytesRead);

	item->array = false;

	if (!bytesRead) {
		data->eof = true;
		type = AL_TOKEN_EOF;

	} else {
		switch (type) {
			case AL_TOKEN_START:
			case AL_TOKEN_END:
				break;

			case AL_TOKEN_TAG: TRY(read_tag(data, &item->value.tag)); break;
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
			case AL_VAR_BLOB: TRY(read_blob(data, &item->value.blob)); break;

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
				al_log_error("unknown value type: 0x%02x", type);
				THROW(AL_ERROR_INVALID_DATA);
		}
	}

	item->type = type;

	PASS()
}

AlError al_data_read_start(AlData *data, bool *atEnd)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));

	if (item.type == AL_TOKEN_END) {
		if (atEnd) {
			*atEnd = true;
		} else {
			al_log_error("unexpected end of group");
			THROW(AL_ERROR_INVALID_DATA);
		}

	} else if (item.type != AL_TOKEN_START) {
		al_log_error("unexpected value, type: 0x%02x", item.type);
		THROW(AL_ERROR_INVALID_DATA);

	} else if (atEnd) {
		*atEnd = false;
	}

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
			if (item.type != AL_TOKEN_TAG) {
				al_log_error("missing expected tag");
				THROW(AL_ERROR_INVALID_DATA);
			}

			if (expected != AL_ANY_TAG && item.value.tag != expected) {
				al_log_error("unexpected tag: 0x%04x", item.value.tag);
				THROW(AL_ERROR_INVALID_DATA);
			}

			if (actual) {
				*actual = item.value.tag;
			}
			break;

		case AL_TOKEN_END:
			if (expected != AL_ANY_TAG) {
				al_log_error("unexpected end of group");
				THROW(AL_ERROR_INVALID_DATA);
			}

			*actual = AL_NO_TAG;
			break;

		default:
			al_log_error("unexpected value, type: 0x%02x", item.type);
			THROW(AL_ERROR_INVALID_DATA);
	}
	
	PASS()
}

AlError al_data_read_value(AlData *data, AlVarType type, void *value, bool *atEnd)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));

	if (item.type == AL_TOKEN_END) {
		if (atEnd) {
			*atEnd = true;
		} else {
			al_log_error("unexpected end of group");
			THROW(AL_ERROR_INVALID_DATA);
		}

	} else if (item.type != type) {
		al_log_error("value is unexpected type: 0x%02x, expecting: 0x%02x", item.type, type);
		THROW(AL_ERROR_INVALID_DATA);

	} else if (item.array) {
		al_log_error("unexpected array");
		THROW(AL_ERROR_INVALID_DATA);

	} else {
		if (atEnd) {
			*atEnd = false;
		}

		switch (type) {
			case AL_VAR_BOOL: *(bool *)value = item.value.boolVal; break;
			case AL_VAR_INT: *(int *)value = item.value.intVal; break;
			case AL_VAR_DOUBLE: *(double *)value = item.value.doubleVal; break;
			case AL_VAR_VEC2: *(Vec2 *)value = item.value.vec2; break;
			case AL_VAR_VEC3: *(Vec3 *)value = item.value.vec3; break;
			case AL_VAR_VEC4: *(Vec4 *)value = item.value.vec4; break;
			case AL_VAR_BOX2: *(Box2 *)value = item.value.box2; break;
			case AL_VAR_STRING: *(char **)value = item.value.string.chars; break;
			case AL_VAR_BLOB: *(AlBlob *)value = item.value.blob; break;
		}
	}

	PASS()
}

AlError al_data_read_array(AlData *data, AlVarType type, void *values, uint64_t *count, bool *atEnd)
{
	BEGIN()

	AlDataItem item;
	TRY(al_data_read(data, &item));

	if (item.type == AL_TOKEN_END) {
		if (atEnd) {
			*atEnd = true;
		} else {
			al_log_error("unexpected end of group");
			THROW(AL_ERROR_INVALID_DATA);
		}

	} else if (item.type != type) {
		al_log_error("value is unexpected type: 0x%02x, expecting: 0x%02x", item.type, type);
		THROW(AL_ERROR_INVALID_DATA);

	} else if (!item.array) {
		al_log_error("unexpected single value");
		THROW(AL_ERROR_INVALID_DATA);

	} else {
		if (atEnd) {
			*atEnd = false;
		}

		*(void **)values = item.value.array.items;
		*count = item.value.array.length;
		data->temp = NULL;
	}

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
			case AL_VAR_INT: TRY(skip_uint(data)); break;
			case AL_VAR_DOUBLE: TRY(data_seek(data, 8, AL_SEEK_CUR)); break;
			case AL_VAR_VEC2: TRY(data_seek(data, 16, AL_SEEK_CUR)); break;
			case AL_VAR_VEC3: TRY(data_seek(data, 24, AL_SEEK_CUR)); break;
			case AL_VAR_VEC4: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;
			case AL_VAR_BOX2: TRY(data_seek(data, 32, AL_SEEK_CUR)); break;
			case AL_VAR_STRING: TRY(skip_string(data)); break;
			case AL_VAR_BLOB: TRY(skip_blob(data)); break;

			case AL_VAR_BOOL | 0x80:
			case AL_VAR_INT | 0x80:
			case AL_VAR_DOUBLE | 0x80:
			case AL_VAR_VEC2 | 0x80:
			case AL_VAR_VEC3 | 0x80:
			case AL_VAR_VEC4 | 0x80:
			case AL_VAR_BOX2 | 0x80:
				TRY(skip_array(data, type & 0x7F));
				break;

			default:
				al_log_error("unknown value type: 0x%02x", type);
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
	TRY(write_tag(data, &tag));

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
		case AL_VAR_BLOB: TRY(write_blob(data, value)); break;
		default:
			al_log_error("unknown value type: 0x%02x", type);
			THROW(AL_ERROR_INVALID_OPERATION);
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

AlError al_data_write_array(AlData *data, AlVarType type, const void *values, uint64_t count)
{
	BEGIN()

	TRY(write_type(data, type | 0x80));
	TRY(write_array(data, type, values, count));

	PASS()
}
