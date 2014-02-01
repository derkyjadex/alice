/*
 * Copyright (c) 2014 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_TRIPLE_BUFFER_H__
#define __ALBASE_TRIPLE_BUFFER_H__

#include "albase/common.h"

typedef struct AlTripleBuffer AlTripleBuffer;

AlError al_triple_buffer_init(AlTripleBuffer **buffer, size_t size, const void *initial);
void al_triple_buffer_free(AlTripleBuffer *buffer);

const void *al_triple_buffer_read(AlTripleBuffer *buffer);
void *al_triple_buffer_write(AlTripleBuffer *buffer);
void al_triple_buffer_flip(AlTripleBuffer *buffer);

#endif
