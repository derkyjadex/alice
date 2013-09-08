/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <unistd.h>
#include <sys/param.h>

#include <CoreFoundation/CoreFoundation.h>

#include "albase/fs.h"

AlError al_fs_chdir_app_data()
{
	BEGIN()

	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);

	char resourcesDir[MAXPATHLEN];
	Boolean result = CFURLGetFileSystemRepresentation(url, true, (UInt8 *)resourcesDir, MAXPATHLEN);

	if (!result)
		THROW(AL_ERROR_GENERIC);

	chdir(resourcesDir);

	PASS(
		CFRelease(url);
	)
}
