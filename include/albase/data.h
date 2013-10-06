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

/** Type used to tag groups in the alice data format */
typedef uint32_t AlDataTag;

#define AL_NO_TAG ((AlDataTag)0)
#define AL_ANY_TAG ((AlDataTag)UINT32_MAX)

/** Create a tag from four characters */
#define AL_DATA_TAG(a, b, c, d) ((AlDataTag)((((d) << 24) | ((c) << 16) | ((b) << 8) | (a))))

/**
 * Read a sequence of tagged groups until the end of the parent group.
 * Use case labels with the expected tags to read their contents. Use
 * al_data_skip_rest() after reading the expected items to read to the end of
 * the current group.
 * Unknown tags are skipped over. End the block with END_READ_TAGS.
 * @param data The AlData object to read from
 */
#define START_READ_TAGS(data) { \
	bool data##AtEnd = false; \
	do { \
		AlDataTag tag; \
		TRY(al_data_read_start_tag(data, AL_ANY_TAG, &tag)); \
		 \
		switch (tag) { \
			case AL_NO_TAG: \
				data##AtEnd = true; \
				break; \

/**
 * End a block started with START_READ_TAGS
 * @param data The AlData object being read from
 */
#define END_READ_TAGS(data) \
			default: \
				TRY(al_data_skip_rest(data)); \
		} \
	} while (!data##AtEnd); \
}

/**
 * Use to break out of a block started with START_READ_TAGS
 * @param data The AlData object being read from
 */
#define BREAK_READ_TAGS(data) \
	data##AtEnd = true; \
	break;

typedef enum {
	/** The start of a group */
	AL_TOKEN_START = 0xFE,
	/** The end of a group */
	AL_TOKEN_END = 0xEF,
	/** A tag item */
	AL_TOKEN_TAG = 0xEE,
	/** The end of the AlStream */
	AL_TOKEN_EOF = 0xFF
} AlToken;

/** A single item from an alice data stream */
typedef struct {
	/** Can be an AlToken or an AlVarType */
	uint8_t type;
	bool array;
	union {
		AlDataTag tag;
		bool boolVal;
		int32_t intVal;
		double doubleVal;
		Vec2 vec2;
		Vec3 vec3;
		Vec4 vec4;
		Box2 box2;
		struct {
			char *chars;
			uint64_t length;
		} string;
		AlBlob blob;
		struct {
			void *items;
			uint64_t length;
		} array;
	} value;
} AlDataItem;

/** Reads and writes streams in the alice data format */
typedef struct AlData AlData;

/**
 * Create a new AlData object.
 * @param[out] data Pointer to where the new AlData object pointer will be
 * written
 * @param stream The stream that will be written to/read from
 */
AlError al_data_init(AlData **data, AlStream *stream);

/**
 * Free an AlData object. Does not free its stream.
 */
void al_data_free(AlData *data);

/**
 * Read a single item from the stream.
 * @param[out] item Pointer to where the result will be written
 */
AlError al_data_read(AlData *data, AlDataItem *item);

/**
 * Read and expect the start of a group.
 * Returns an error if the start of a group was not read. If atEnd is not
 * NULL, the end of a group will be accepted and atEnd set to true.
 * @param[out] atEnd If not NULL, set to true if at the end of a group
 */
AlError al_data_read_start(AlData *data, bool *atEnd);

/**
 * Read and expect the start of a tagged group.
 * Can be used to read through a sequence of tagged groups within a parent
 * group, or to read the start of a specific expected tagged group. Returns an 
 * error if a tagged group was not read, or the tag was not the expected one.
 * @param expected The expected tag, or AL_ANY_TAG to accept any
 * @param[out] actual The tag that was read, or AL_NO_TAG if at the end of the
 * parent group
 */
AlError al_data_read_start_tag(AlData *data, AlDataTag expected, AlDataTag *actual);

/**
 * Read a single value of an expected type.
 * Returns an error if the item read was not a single value of the specified
 * type. If atEnd is not NULL, the end of a group will be accepted and atEnd set
 * to true.
 * @param type The expected type to read
 * @param[out] value Pointer to write the value to
 * @param[out] atEnd If not NULL, set to true if at the end of a group
 */
AlError al_data_read_value(AlData *data, AlVarType type, void *value, bool *atEnd);

/**
 * Read an array of an expected type.
 * Returns an error if the item read was not an array of the specified type.
 * If atEnd is not NULL, the end of a group will be accepted and atEnd set to
 * true.
 * @param type The expected type to read
 * @param[out] values Pointer to write the array to
 * @param[out] count Pointer to write the array length to
 * @param[out] atEnd If not NULL, set to true if at the end of a group
 */
AlError al_data_read_array(AlData *data, AlVarType type, void *values, uint64_t *count, bool *atEnd);

/**
 * Read over items until the end of a group is reached.
 */
AlError al_data_skip_rest(AlData *data);

/**
 * Write the start of a group.
 */
AlError al_data_write_start(AlData *data);

/**
 * Write the end of a group.
 */
AlError al_data_write_end(AlData *data);

/**
 * Write the start of a tagged group.
 * @param tag The tag for the group
 */
AlError al_data_write_start_tag(AlData *data, AlDataTag tag);

/**
 * Write a tagged group with a single value.
 * @param tag The tag for the group
 * @param type The type of the value
 * @param value Pointer to the value to be written
 */
AlError al_data_write_simple_tag(AlData *data, AlDataTag tag, AlVarType type, const void *value);

/**
 * Write a single value.
 * strlen() is used to find the length of strings.
 * @param type The type of the value
 * @param value Pointer to the value to be written
 */
AlError al_data_write_value(AlData *data, AlVarType type, const void *value);

/**
 * Write a string with the length specified.
 * @oaram value Pointer to the start of the string
 * @param length The length of the string
 */
AlError al_data_write_string(AlData *data, const char *value, uint32_t length);

/**
 * Write an array.
 * Arrays of strings or blobs are not supported.
 * @param type The type of the values in the array
 * @param values Pointer to the start of the array
 * @param count The length of the array
 */
AlError al_data_write_array(AlData *data, AlVarType type, const void *values, uint64_t count);

#endif
