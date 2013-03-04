/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef MODEL_SHAPE_INTERNAL_H
#define MODEL_SHAPE_INTERNAL_H

struct AlModelShape {
	int numPaths;
	size_t pathsLength;
	AlModelPath **paths;
};

struct AlModelPath {
	Vec3 colour;
	int numPoints;
	size_t pointsLength;
	Vec2 *points;
};

#endif
