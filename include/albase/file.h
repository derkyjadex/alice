/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_FILE_H__
#define __ALBASE_FILE_H__

#include "common.h"

typedef enum {
	OPEN_READ,
	OPEN_WRITE
} AlOpenMode;

AlError al_file_open(FILE **file, const char *filename, AlOpenMode mode);
void al_file_close(FILE *file);

AlError al_file_read(FILE *file, void *ptr, size_t size, size_t count);
AlError al_file_write(FILE *file, const void *ptr, size_t size, size_t count);

AlError al_file_read_string(FILE *file, char **string);
AlError al_file_write_string(FILE *file, const char *string);

AlError al_file_read_array(FILE *file, void *array, int *count, size_t size);
AlError al_file_write_array(FILE *file, const void *array, int count, size_t size);

AlError al_read_file_to_string(const char *filename, char **string);

#endif
