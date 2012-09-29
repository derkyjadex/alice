/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_MODEL_H__
#define __ALBASE_GL_MODEL_H__

#include "albase/model.h"
#include "albase/common.h"
#include "albase/geometry.h"
#include "albase/gl/opengl.h"

struct AlModel {
	struct AlModel *next;
	struct AlModel *prev;
	char *filename;
	int users;

	int numPaths;
	GLuint vertexBuffer;
	int *vertexCounts;
	Vec3 *colours;

	Box bounds;
};

typedef struct AlGlModelVertex {
	struct { float x, y; } position;
	struct { float x, y; } normal;
} AlGlModelVertex;

#endif
