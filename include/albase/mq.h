/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_MQ_H__
#define __ALBASE_MQ_H__

#include <stdbool.h>

#include "albase/common.h"

typedef struct AlMQ AlMQ;

AlError al_mq_init(AlMQ **result, size_t messageSize, size_t size);
void al_mq_free(AlMQ *mq);

bool al_mq_push(AlMQ *mq, const void *message);
bool al_mq_push_locked(AlMQ *mq, const void *message);
bool al_mq_pop(AlMQ *mq, void *message);

#endif
