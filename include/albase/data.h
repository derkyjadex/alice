/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_DATA_H__
#define __ALBASE_DATA_H__

#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "stream.h"
#include "geometry.h"
#include "vars.h"

#define AL_DATA_TAG(a, b, c, d) ((((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

typedef int32_t AlDataTag;

static const AlDataTag AL_NO_TAG = 0;
static const AlDataTag AL_ANY_TAG = 0;

typedef enum {
	AL_TOKEN_START = 0xFE,
	AL_TOKEN_END = 0xEF,
} AlToken;

typedef struct {
	uint8_t type;
	bool array;
	union {
		bool boolVal;
		int32_t intVal;
		double doubleVal;
		Vec2 vec2;
		Vec3 vec3;
		Vec4 vec4;
		Box box;
		struct {
			uint32_t length;
			char *chars;
		} string;
		struct {
			uint32_t length;
			void *items;
		} array;
	} value;
} AlDataItem;

typedef struct AlData AlData;

AlError al_data_init(AlData **data, AlStream *stream);
void al_data_free(AlData *data);

AlError al_data_read(AlData *data, AlDataItem *item);
AlError al_data_read_start(AlData *data);
AlError al_data_read_start_tag(AlData *data, AlDataTag expected, AlDataTag *actual);
AlError al_data_read_value(AlData *data, AlVarType type, void *value);
AlError al_data_read_array(AlData *data, AlVarType type, void *values, uint32_t *count);
AlError al_data_skip_rest(AlData *data);

AlError al_data_write_start(AlData *data);
AlError al_data_write_end(AlData *data);
AlError al_data_write_start_tag(AlData *data, AlDataTag tag);
AlError al_data_write_simple_tag(AlData *data, AlDataTag tag, AlVarType type, void *value);
AlError al_data_write_value(AlData *data, AlVarType type, void *value);
AlError al_data_write_string(AlData *data, const char *value, uint32_t length);
AlError al_data_write_array(AlData *data, AlVarType type, void *values, uint32_t count);

#endif
