/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef __ALBASE_WRAPPER_H__
#define __ALBASE_WRAPPER_H__

#include "albase/common.h"
#include "albase/lua.h"
#include "albase/commands.h"

typedef struct AlWrapper AlWrapper;
typedef void (*AlWrapperFree)(lua_State *L, void *ptr);

AlError al_wrapper_init(AlWrapper **wrapper, lua_State *L, size_t objSize, AlWrapperFree free);
void al_wrapper_free(AlWrapper *wrapper);

AlError al_wrapper_register_ctor(AlWrapper *wrapper);
AlError al_wrapper_register_register_ctor_command(AlWrapper *wrapper, const char *name, AlCommands *commands);
AlError al_wrapper_invoke_ctor(AlWrapper *wrapper, void *result);
AlError al_wrapper_create(AlWrapper *wrapper, void *result);
void al_wrapper_retain(AlWrapper *wrapper, void *obj);
void al_wrapper_release(AlWrapper *wrapper, void *obj);

void al_wrapper_push_userdata(AlWrapper *wrapper, void *obj);

void al_wrapper_reference(AlWrapper *wrapper);
void al_wrapper_unreference(AlWrapper *wrapper);

#endif
