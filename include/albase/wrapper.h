/*
 * Copyright (c) 2012-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */


#ifndef __ALBASE_WRAPPER_H__
#define __ALBASE_WRAPPER_H__

#include "albase/common.h"
#include "albase/lua.h"

/** Handle for types registered with the wrapper */
typedef struct AlWrappedType AlWrappedType;

/** Registration info for a wrapped type */
typedef struct {
	/** The unique name of the type */
	const char *name;
	/** Size of the memory to be allocated for each instance created */
	size_t size;

	/**
	 * Function to be called to initialize a new instance.
	 * @param L The Lua state
	 * @param ptr Pointer to the new object to initialize
	 * @param data Pointer to custom data
	 * */
	AlError (*init)(lua_State *L, void *ptr, void *data);

	/** Custom data to be passed to init function */
	void *initData;

	/**
	 * Function to be called to free an object.
	 * The object iself should not be freed.
	 * @param L The Lua state
	 * @param ptr The object to free
	 */
	void (*free)(lua_State *L, void *ptr);
} AlWrapperReg;

/**
 * Initialize a wrapper for the lua_State.
 * The wrapper is active for the lifetime of the Lua state.
 * When the Lua state is closed, all wrapped objects are freed in
 * the reverse of the order they were created in.
 * @param L The Lua state
 */
AlError al_wrapper_init(lua_State *L);

/**
 * Register a wrapped type.
 * @param L The Lua state
 * @param reg Registration info for the type
 * @param[out] type Pointer to the registered wrapped type
 */
AlError al_wrapper_register(lua_State *L, AlWrapperReg reg, AlWrappedType **type);

/**
 * Create a new instance of a wrapped type.
 * Implicitly calls al_wrapper_retain() once before returning
 * the object.
 * @param type The wrapped type
 * @param[out] result Pointer to the new object
 */
AlError al_wrapper_invoke_ctor(AlWrappedType *type, void *result);

/**
 * Increment the retain count for a wrapped object.
 * Wrapped objects are subject to normal Lua garbage collection,
 * but Lua does not know about references from C. Retaining an
 * object causes it to be kept alive even if there are no remaining
 * references to it from Lua. Should be matched with a call to
 * al_wrapper_release().
 * @param L The Lua state
 * @param ptr The object to retain
 */
void al_wrapper_retain(lua_State *L, void *ptr);

/**
 * Decrement the retain count for a wrapped object.
 * The wrapped object is not freed immediately when the retain
 * count reaches zero, but it becomes eligible for regular
 * Lua garbage collection.
 * @param L The Lua state
 * @param ptr The object to release
 */
void al_wrapper_release(lua_State *L, void *ptr);

/**
 * Push full userdata onto the Lua stack.
 * @param L The Lua state
 * @param ptr The object to push to the stack
 */
void al_wrapper_push_userdata(lua_State *L, void *ptr);

/**
 * Record a reference from the wrapped object at -2 on the stack
 * to the wrapped object at -1.
 * Call this function to make Lua aware of a reference from one
 * wrapped object to another outside of Lua. This will prevent the
 * second object being collected while the first is still alive.
 * Wrapped objects can reference each other multiple times. Should
 * be matched with calls to al_wrapper_unreference().
 * @param L The Lua state
 */
void al_wrapper_reference(lua_State *L);

/**
 * Remove a reference from the wrapped object at -2 on the stack
 * to the wrapped object at -1.
 * @param L The Lua state
 */
void al_wrapper_unreference(lua_State *L);

#endif
