/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "albase/common.h"
#include "file_system.h"
#include "albase/lua.h"

static int cmd_get_cwd(lua_State *L)
{
	char buffer[PATH_MAX];
	char *result = getcwd(buffer, sizeof(buffer));
	if (!result) {
		return luaL_error(L, "fs_get_cwd: path too long for buffer");
	}

	lua_pushstring(L, buffer);

	return 1;
}

static bool is_hidden(const char *filename)
{
	return filename[0] == '.' && filename[1] != '.';
}

static void resolve_path(lua_State *L, const char *path, const char *filename)
{
	char fullPath[PATH_MAX];
	int numChars = snprintf(fullPath, sizeof(fullPath), "%s/%s", path, filename);

	if (numChars < 0 || numChars >= sizeof(fullPath)) {
		luaL_error(L, "resolve_path: error resolving path %s/%s", path, filename);
	}

	char resolvedPath[PATH_MAX];
	char *result = realpath(fullPath, resolvedPath);
	if (!result) {
		luaL_error(L, "resolve_path: error resolving path %s", fullPath);
	}

	lua_pushstring(L, resolvedPath);
}

static int cmd_list_dir(lua_State *L)
{
	DIR *dir = NULL;
	struct dirent *entry;

	const char *path =luaL_checkstring(L, 1);

	dir = opendir(path);
	if (!dir) {
		return luaL_error(L, "fs_get_files: failed to open directory");
	}

	int numEntries = 0;
	while ((entry = readdir(dir))) {
		if (is_hidden(entry->d_name))
			continue;

		lua_newtable(L);
		lua_pushstring(L, "name");
		lua_pushstring(L, entry->d_name);
		lua_settable(L, -3);

		lua_pushstring(L, "path");
		resolve_path(L, path, entry->d_name);
		lua_settable(L, -3);

		if (entry->d_type & DT_DIR) {
			lua_pushstring(L, "is_dir");
			lua_pushboolean(L, true);
			lua_settable(L, -3);
		}

		numEntries++;
	}

	closedir(dir);

	return numEntries;
}

AlError file_system_register_commands(AlCommands *commands)
{
	BEGIN()

	TRY(al_commands_register(commands, "fs_get_cwd", cmd_get_cwd, NULL));
	TRY(al_commands_register(commands, "fs_list_dir", cmd_list_dir, NULL));

	PASS()
}
