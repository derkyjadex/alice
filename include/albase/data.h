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

typedef enum {
	AL_TOKEN_START = 0xFE,
	AL_TOKEN_END = 0xEF,
	AL_TOKEN_EOF
} AlToken;

typedef struct {
	uint8_t type;
	union {
		bool boolVal;
		int intVal;
		double doubleVal;
		Vec2 vec2;
		Vec3 vec3;
		Vec4 vec4;
		Box box;
		struct {
			int length;
			const char *chars;
		} string;
	} value;
} AlDataItem;

void al_data_item_init(AlDataItem *item);
void al_data_item_free(AlDataItem *item);

AlError al_data_read(AlStream *stream, AlDataItem *item);
AlError al_data_read_rest(AlStream *stream, AlVarType type, void *array, int *count);
AlError al_data_skip_rest(AlStream *stream);

AlError al_data_write_start(AlStream *stream);
AlError al_data_write_end(AlStream *stream);
AlError al_data_write_value(AlStream *stream, AlVarType type, void *value);
AlError al_data_write_values(AlStream *stream, AlVarType type, void *values, int count);
AlError al_data_write_string(AlStream *stream, const char *value, int length);

#endif
