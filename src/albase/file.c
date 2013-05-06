/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "albase/file.h"

AlError al_file_open(FILE **result, const char *filename, AlOpenMode mode)
{
	BEGIN()

	FILE *file = NULL;
	file = fopen(filename, (mode == AL_OPEN_READ) ? "rb" : "w");
	if (file == NULL) {
		al_log_error("Could not open file: '%s'", filename);
		THROW(AL_ERROR_IO)
	}

	*result = file;

	PASS()
}

void al_file_close(FILE *file)
{
	if (file != NULL) {
		fclose(file);
	}
}

AlError al_file_read(FILE *file, void *ptr, size_t size, size_t count)
{
	BEGIN()

	if (fread(ptr, size, count, file) != count) {
		THROW(AL_ERROR_IO)
	}

	PASS()
}

AlError al_file_write(FILE *file, const void *ptr, size_t size, size_t count)
{
	BEGIN()

	if (fwrite(ptr, size, count, file) == 0) {
		THROW(AL_ERROR_IO)
	}

	PASS()
}

AlError al_file_read_string(FILE *file, char **result)
{
	BEGIN()

	int length;
	char *string = NULL;

	TRY(al_file_read(file, &length, sizeof(int), 1));
	TRY(al_malloc(&string, sizeof(char), length + 1));
	TRY(al_file_read(file, string, sizeof(char), length));
	string[length] = '\0';

	*result = string;

	CATCH(
		free(string);
	)
	FINALLY()
}

AlError al_file_write_string(FILE *file, const char *string)
{
	BEGIN()

	size_t length = strlen(string);
	TRY(al_file_write(file, &length, sizeof(int), 1));
	TRY(al_file_write(file, string, sizeof(char), length));

	PASS()
}

AlError al_file_read_array(FILE *file, void *result, int *resultCount, size_t size)
{
	BEGIN()

	int count;
	void *array = NULL;

	TRY(al_file_read(file, &count, sizeof(int), 1));
	TRY(al_malloc(&array, size, count));
	TRY(al_file_read(file, array, size, count));

	*(void**)result = array;
	*resultCount = count;

	CATCH(
		free(array);
	)
	FINALLY()
}

AlError al_file_write_array(FILE *file, const void *array, int count, size_t size)
{
	BEGIN()

	TRY(al_file_write(file, &count, sizeof(int), 1));
	TRY(al_file_write(file, array, size, count));

	PASS()
}

AlError al_read_file_to_string(const char *filename, char **result)
{
	BEGIN()

	FILE *file = NULL;
	char *string = NULL;

	TRY(al_file_open(&file, filename, AL_OPEN_READ));

	fseek(file, 0, SEEK_END);
	size_t length = ftell(file);
	fseek(file, 0, SEEK_SET);

	TRY(al_malloc(&string, sizeof(char), length + 1));
	TRY(al_file_read(file, string, sizeof(char), length));
	string[length] = '\0';

	*result = string;

	CATCH(
		free(string);
	)
	FINALLY(
		al_file_close(file);
	)
}
