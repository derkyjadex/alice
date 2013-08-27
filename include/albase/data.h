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

typedef int32_t AlDataTag;

#define AL_NO_TAG ((AlDataTag)0)
#define AL_ANY_TAG ((AlDataTag)0)
#define AL_DATA_TAG(a, b, c, d) ((AlDataTag)((((d) << 24) | ((c) << 16) | ((b) << 8) | (a))))

#define READ_TAGS(data, x) { \
	bool atEnd = false; \
	do { \
		AlDataTag tag; \
		TRY(al_data_read_start_tag(data, AL_ANY_TAG, &tag)); \
		 \
		switch (tag) { \
			case AL_NO_TAG: \
				atEnd = true; \
				break; \
			 \
			x \
			 \
			default: \
				TRY(al_data_skip_rest(data)); \
		} \
	} while (!atEnd); \
}

#define CASE_TAG(tag, x) \
	case tag: { \
			x \
		} \
		break;

#define BREAK_READ_TAGS \
	atEnd = true; \
	break;

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
		Box2 box2;
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
AlError al_data_write_simple_tag(AlData *data, AlDataTag tag, AlVarType type, const void *value);
AlError al_data_write_value(AlData *data, AlVarType type, const void *value);
AlError al_data_write_string(AlData *data, const char *value, uint32_t length);
AlError al_data_write_array(AlData *data, AlVarType type, const void *values, uint32_t count);

#endif
