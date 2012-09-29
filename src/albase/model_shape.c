/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "albase/model_shape.h"
#include "albase/file.h"

AlError al_model_shape_init(AlModelShape **result)
{
	BEGIN()

	AlModelShape *shape = NULL;
	TRY(al_malloc(&shape, sizeof(AlModelShape), 1));

	shape->numPaths = 0;
	shape->pathsLength = 0;
	shape->paths = NULL;

	TRY(al_malloc(&shape->paths, sizeof(AlModelPath), 4));
	shape->pathsLength = 4;

	*result = shape;

	CATCH(
		al_model_shape_free(shape);
	)
	FINALLY()
}

void al_model_shape_free(AlModelShape *shape)
{
	if (shape) {
		for (int i = 0; i < shape->numPaths; i++) {
			free(shape->paths[i].points);
		}
		free(shape->paths);
		free(shape);
	}
}

AlError al_model_shape_load(AlModelShape *shape, const char *filename)
{
	BEGIN()

	FILE *file = NULL;
	int numPaths = 0;
	AlModelPath *paths = NULL;

	TRY(al_file_open(&file, filename, OPEN_READ));
	TRY(al_file_read(file, &numPaths, sizeof(int), 1));
	TRY(al_malloc(&paths, sizeof(AlModelPath), numPaths));

	for (int i = 0; i < numPaths; i++) {
		paths[i].numPoints = 0;
		paths[i].pointsLength = 0;
		paths[i].points = NULL;

		TRY(al_file_read(file, &paths[i].colour, sizeof(Vec3), 1));
		TRY(al_file_read_array(file, &paths[i].points, &paths[i].numPoints, sizeof(Vec2)));
	}

	shape->numPaths = numPaths;
	shape->pathsLength = numPaths;
	shape->paths = paths;

	CATCH(
		al_log_error("Error reading model file");
		if (paths) {
		  for (int i = 0; i < numPaths; i++) {
			  if (paths[i].points == NULL) break;
			  free(paths[i].points);
		  }
		  free(paths);
		}
	)
	FINALLY(
		al_file_close(file);
	)
}

AlError al_model_shape_save(AlModelShape *shape, const char *filename)
{
	BEGIN()

	FILE *file = NULL;

	TRY(al_file_open(&file, filename, OPEN_WRITE));
	TRY(al_file_write(file, &shape->numPaths, sizeof(int), 1));

	for (int i = 0; i < shape->numPaths; i++) {
		TRY(al_file_write(file, &shape->paths[i].colour, sizeof(Vec3), 1));
		TRY(al_file_write_array(file, shape->paths[i].points, shape->paths[i].numPoints, sizeof(Vec2)));
	}

	PASS(
		al_file_close(file);
	)
}

AlError al_model_shape_add_path(AlModelShape *shape, int index, Vec2 start, Vec2 end)
{
	assert(index >= -1 && index <= shape->numPaths);

	BEGIN()

	Vec2 *points = NULL;

	if (index == -1)
		index = shape->numPaths;

	if (shape->numPaths == shape->pathsLength) {
		TRY(al_realloc(&shape->paths, sizeof(AlModelPath), shape->pathsLength * 2));
		shape->pathsLength *= 2;
	}

	TRY(al_malloc(&points, sizeof(Vec2), 2));

	for (int i = shape->numPaths; i > index; i--) {
		shape->paths[i] = shape->paths[i - 1];
	}

	points[0] = start;
	points[1] = end;

	shape->paths[index].colour = (Vec3){1.0, 1.0, 1.0};
	shape->paths[index].numPoints = 2;
	shape->paths[index].pointsLength = 2;
	shape->paths[index].points = points;

	shape->numPaths++;

	PASS()
}

AlError model_shape_remove_path(AlModelShape *shape, int index)
{
	assert(index >= 0 && index < shape->numPaths);

	for (int i = index; i < shape->numPaths; i++) {
		shape->paths[i] = shape->paths[i + 1];
	}

	shape->numPaths--;

	return AL_NO_ERROR;
}

AlError al_model_path_add_point(AlModelPath *path, int index, Vec2 location)
{
	assert(index >= -1 && index <= path->numPoints);

	BEGIN()

	if (index == -1)
		index = path->numPoints;

	if (path->numPoints == path->pointsLength) {
		TRY(al_realloc(&path->points, sizeof(Vec2), path->pointsLength * 2));
		path->pointsLength *= 2;
	}

	for (int i = path->numPoints; i > index; i--) {
		path->points[i] = path->points[i - 1];
	}

	path->points[index] = location;

	path->numPoints++;

	PASS()
}

AlError al_model_path_remove_point(AlModelPath *path, int index)
{
	assert(index >= 0 && index < path->numPoints);
	assert(path->numPoints > 2);

	for (int i = index; i < path->numPoints; i++) {
		path->points[i] = path->points[i + 1];
	}

	path->numPoints--;

	return AL_NO_ERROR;
}
