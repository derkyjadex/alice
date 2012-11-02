/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef __ALBASE_WRAPPER_H__
#define __ALBASE_WRAPPER_H__

#include "albase/common.h"
#include "albase/lua.h"

typedef struct AlWrapper AlWrapper;
typedef void (*AlWrapperFree)(lua_State *L, void *ptr);

AlError al_wrapper_init(AlWrapper **wrapper, lua_State *L, bool weak, AlWrapperFree free);
void al_wrapper_free(AlWrapper *wrapper);

AlError al_wrapper_register_ctor(AlWrapper *wrapper);
AlError al_wrapper_register(AlWrapper *wrapper, void *ptr, int tableN);
void al_wrapper_unregister(AlWrapper *wrapper, void *ptr);

void *al_wrapper_unwrap(AlWrapper *wrapper);
void al_wrapper_wrap(AlWrapper *wrapper, void *ptr, int nArgs);

#endif
