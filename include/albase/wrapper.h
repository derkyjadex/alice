/*
 * Copyright (c) 2012-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef __ALBASE_WRAPPER_H__
#define __ALBASE_WRAPPER_H__

#include "albase/common.h"
#include "albase/lua.h"

typedef struct AlWrapper AlWrapper;
typedef struct AlWrappedType AlWrappedType;

typedef struct {
	const char *name;
	size_t size;
	AlError (*init)(lua_State *L, void *ptr, void *data);
	void *initData;
	void (*free)(lua_State *L, void *ptr);
} AlWrapperReg;

AlError al_wrapper_init(AlWrapper **wrapper, lua_State *L);
void al_wrapper_free(AlWrapper *wrapper);

AlError al_wrapper_register(AlWrapper *wrapper, AlWrapperReg reg, AlWrappedType **type);
AlError al_wrapper_invoke_ctor(AlWrappedType *type, void *result);

void al_wrapper_retain(AlWrapper *wrapper, void *ptr);
void al_wrapper_release(AlWrapper *wrapper, void *ptr);

void al_wrapper_push_userdata(AlWrapper *wrapper, void *ptr);

void al_wrapper_reference(AlWrapper *wrapper);
void al_wrapper_unreference(AlWrapper *wrapper);

#endif
