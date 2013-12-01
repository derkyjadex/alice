/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "albase/script.h"
#include "scripts.h"
#include "albase/lua.h"
#include "libs.h"

static AlLuaKey tracebackKey;

static int traceback(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 1;

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);

	return 1;
}

static void *alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	AlError error = al_realloc(&ptr, nsize);
	if (error)
		ptr = NULL;

	return ptr;
}

AlError al_script_init(lua_State **result)
{
	BEGIN()

	lua_State *L = NULL;

	L = lua_newstate(alloc, NULL);
	if (!L) {
		al_log_error("failed to create Lua state");
		THROW(AL_ERROR_MEMORY)
	}

	luaL_openlibs(L);

	luaL_Reg luaLibs[] = {
		{"fs", luaopen_fs},
		{"text", luaopen_text},
		{NULL, NULL}
	};

	for (luaL_Reg *lib = luaLibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, false);
		lua_pop(L, 1);
	}

	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushcclosure(L, traceback, 1);
	lua_pushlightuserdata(L, &tracebackKey);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 2);

	*result = L;

	CATCH(
		lua_close(L);
	)
	FINALLY()
}

static AlError run_script(lua_State *L, int loadResult)
{
	BEGIN()

	if (loadResult == LUA_OK) {
		al_script_push_traceback(L);
		lua_pushvalue(L, -2);

		loadResult = lua_pcall(L, 0, 0, -2);
	}

	if (loadResult != LUA_OK) {
		const char *message = lua_tostring(L, -1);
		al_log_error("error running script: \n%s", message);
		lua_pop(L, 1);
		THROW(AL_ERROR_SCRIPT);
	}

	PASS(
		lua_pop(L, 2);
	)
}

AlError al_script_run_file(lua_State *L, const char *filename)
{
	BEGIN()

	AlStream *stream = NULL;
	TRY(al_stream_init_filename(&stream, filename, AL_OPEN_READ));
	TRY(al_script_run_stream(L, stream));

	PASS(
		al_stream_free(stream);
	)
}

AlError al_script_run_buffer(lua_State *L, const void *ptr, size_t size, const char* name)
{
	BEGIN()

	AlMemStream stream = al_stream_init_mem_stack(ptr, size, name);
	TRY(al_script_run_stream(L, &stream.base));

	PASS()
}

AlError al_script_run_base_scripts(lua_State *L)
{
	BEGIN()

	TRY(AL_SCRIPT_RUN(L, common));
	TRY(AL_SCRIPT_RUN(L, class));
	TRY(AL_SCRIPT_RUN(L, wrapper));
	TRY(AL_SCRIPT_RUN(L, model));

	PASS()
}

#define READ_BUFFER_SIZE 1024

typedef struct {
	AlStream *stream;
	char buffer[READ_BUFFER_SIZE];
	AlError error;
} ReadData;

static const char *stream_reader(lua_State *L, void *data, size_t *size)
{
	ReadData *readData = (ReadData *)data;
	AlStream *stream = readData->stream;
	char *buffer = readData->buffer;

	readData->error = stream->read(stream, buffer, READ_BUFFER_SIZE, size);
	if (readData->error)
		return NULL;

	return buffer;
}

AlError al_script_run_stream(lua_State *L, AlStream *stream)
{
	BEGIN()

	ReadData data = {
		.stream = stream,
		.error = AL_NO_ERROR
	};

	int result = lua_load(L, stream_reader, &data, stream->name, NULL);

	if (data.error) {
		lua_pop(L, 1);
		THROW(data.error);
	}

	TRY(run_script(L, result));

	PASS()
}

void al_script_push_traceback(lua_State *L)
{
	lua_pushlightuserdata(L, &tracebackKey);
	lua_gettable(L, LUA_REGISTRYINDEX);
}

AlError al_script_call(lua_State *L, int nargs)
{
	BEGIN()

	al_script_push_traceback(L);
	lua_insert(L, -nargs - 2);

	int result = lua_pcall(L, nargs, 0, -nargs - 2);
	if (result) {
		const char *message = lua_tostring(L, -1);
		al_log_error("Error calling script: \n%s", message);
		lua_pop(L, 1);
		THROW(AL_ERROR_SCRIPT);
	}

	PASS(
		lua_pop(L, 1);
	)
}
