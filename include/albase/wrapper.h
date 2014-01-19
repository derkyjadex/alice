/*
 * Copyright (c) 2012-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef __ALBASE_WRAPPER_H__
#define __ALBASE_WRAPPER_H__

#include "albase/common.h"
#include "albase/lua.h"

typedef struct AlWrappedType AlWrappedType;

typedef struct {
	const char *name;
	size_t size;
	AlError (*init)(lua_State *L, void *ptr, void *data);
	void *initData;
	void (*free)(lua_State *L, void *ptr);
} AlWrapperReg;

AlError al_wrapper_init(lua_State *L);

AlError al_wrapper_register(lua_State *L, AlWrapperReg reg, AlWrappedType **type);
AlError al_wrapper_invoke_ctor(AlWrappedType *type, void *result);

void al_wrapper_retain(lua_State *L, void *ptr);
void al_wrapper_release(lua_State *L, void *ptr);

void al_wrapper_push_userdata(lua_State *L, void *ptr);

void al_wrapper_reference(lua_State *L);
void al_wrapper_unreference(lua_State *L);

#endif
