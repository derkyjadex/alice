/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_COMMON_H__
#define __ALBASE_COMMON_H__

#include <stdio.h>

#include "albase/error.h"

/**
 * Allocate a memory block.
 * If size is 0, the result is NULL. If the memory cannot be allocated an error
 * is returned.
 * @param[out] ptr Pointer to the pointer that the result is written to
 * @param size The number of bytes to allocate
 */
AlError al_malloc(void *ptr, size_t size);

/**
 * Resize an allocated memory block.
 * If size is 0, the existing memory is freed and ptr is set to NULL. If the
 * reallocation fails, ptr is not changed and an error is returned.
 * @param[out] ptr Pointer to the pointer to the existing block and where the
 * pointer to the resized block is written
 * @param size The new size of the memory block
 */
AlError al_realloc(void *ptr, size_t size);

/**
 * Free an allocated memory block.
 * Frees memory allocated by al_malloc() or al_realloc(). If ptr is NULL, no
 * action is taken.
 * @param ptr Pointer to the memory to free
 */
void al_free(void *ptr);

#endif
