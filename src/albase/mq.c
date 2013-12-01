/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <SDL2/SDL.h>

#include "albase/mq.h"

#define MemoryBarrierAquireRelease() \
	SDL_MemoryBarrierAcquire(); \
	SDL_MemoryBarrierRelease();

struct AlMQ {
	SDL_mutex *lock;
	size_t messageSize;
	size_t size, wrapMask, incrMask;

	volatile size_t start, end;
	char buffer[1];
};

AlError al_mq_init(AlMQ **result, size_t messageSize, size_t size)
{
	BEGIN()

	if (((size - 1) & size) != 0) {
		al_log_error("queue size must be a power of 2");
		THROW(AL_ERROR_GENERIC);
	}

	AlMQ *mq = NULL;
	TRY(al_malloc(&mq, sizeof(AlMQ) - 1 + messageSize * size));

	mq->lock = SDL_CreateMutex();
	if (!mq->lock) {
		al_log_error("failed to create mutex");
		THROW(AL_ERROR_GENERIC);
	}

	mq->messageSize = messageSize;
	mq->size = size;
	mq->wrapMask = size - 1;
	mq->incrMask = 2 * size - 1;
	mq->start = 0;
	mq->end = 0;

	*result = mq;

	CATCH(
		al_mq_free(mq);
	)
	FINALLY()
}

void al_mq_free(AlMQ *mq)
{
	if (mq) {
		SDL_DestroyMutex(mq->lock);
		al_free(mq);
	}
}

bool al_mq_push(AlMQ *mq, const void *message)
{
    if (mq->end == (mq->start ^ mq->size))
		return false;

	size_t index = mq->end & mq->wrapMask;
	void *ptr = mq->buffer + index * mq->messageSize;

	MemoryBarrierAquireRelease();

	memcpy(ptr, message, mq->messageSize);

	SDL_MemoryBarrierRelease();

	mq->end = (mq->end + 1) & mq->incrMask;

    return true;
}

bool al_mq_push_locked(AlMQ *mq, const void *message)
{
	SDL_LockMutex(mq->lock);
	bool result = al_mq_push(mq, message);
	SDL_UnlockMutex(mq->lock);

	return result;
}

bool al_mq_pop(AlMQ *mq, void *message)
{
    if (mq->end == mq->start)
        return false;

	size_t index = mq->start & mq->wrapMask;
    void *ptr = mq->buffer + index * mq->messageSize;

	SDL_MemoryBarrierAcquire();

	memcpy(message, ptr, mq->messageSize);

	MemoryBarrierAquireRelease();

    mq->start = (mq->start + 1) & mq->incrMask;
	
    return true;
}
