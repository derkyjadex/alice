/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_COMMON_H__
#define __ALBASE_COMMON_H__

#include <stdio.h>

#include "albase/error.h"

#define FOR_EACH(type, item, initial) \
	for (type **(item ## Iter) = (initial), *item = *(item ## Iter); item; item = *(++(item ## Iter)))

AlError al_malloc(void *ptr, size_t size, size_t count);
AlError al_realloc(void *ptr, size_t size, size_t count);

#endif
