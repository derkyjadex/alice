/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_COMMON_H__
#define __ALBASE_COMMON_H__

#include <stdio.h>

#include "albase/error.h"

AlError al_malloc(void *ptr, size_t size);
AlError al_realloc(void *ptr, size_t size);

#endif
