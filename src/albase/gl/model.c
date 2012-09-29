/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "albase/gl/model.h"
#include "albase/model_shape.h"
#include "albase/file.h"

static AlModel *firstModel = NULL;

static AlError model_init(AlModel **result)
{
	BEGIN()

	AlModel *model = NULL;
	TRY(al_malloc(&model, sizeof(AlModel), 1));

	model->prev = NULL;
	model->next = NULL;
	model->filename = NULL;
	model->users = 0;

	model->numPaths = 0;
	model->vertexBuffer = 0;
	model->vertexCounts = NULL;
	model->colours = NULL;

	model->bounds = (Box){{0, 0}, {0, 0}};

	glGenBuffers(1, &model->vertexBuffer);

	*result = model;

	PASS()
}

static void model_free(AlModel *model)
{
	if (model != NULL) {
		free(model->filename);
		glDeleteBuffers(1, &model->vertexBuffer);
		free(model->vertexCounts);
		free(model->colours);
		free(model);
	}
}

static AlError model_build_path_vertices(AlGlModelVertex *vertices, AlModelPath *path)
{
	BEGIN()

	Vec2 *points = path->points;

	if (path->numPoints < 2)
		THROW(AL_ERROR_INVALID_DATA)

	Vec2 p1;
	Vec2 p2 = points[0];

	for (int i = 1; i < path->numPoints; i++) {
		p1 = p2;
		p2 = points[i];
		Vec2 n = vec2_normal(vec2_normalise(vec2_subtract(p2, p1)));

		*vertices++ = (AlGlModelVertex){{p1.x, p1.y}, {n.x, n.y}};
		*vertices++ = (AlGlModelVertex){{p1.x, p1.y}, {-n.x, -n.y}};
		*vertices++ = (AlGlModelVertex){{p2.x, p2.y}, {n.x, n.y}};
		*vertices++ = (AlGlModelVertex){{p2.x, p2.y}, {-n.x, -n.y}};
	}

	if (vec2_equals(path->points[0], path->points[path->numPoints - 1])) {
		p1 = points[0];
		p2 = points[1];
		Vec2 n = vec2_normal(vec2_normalise(vec2_subtract(p2, p1)));

		*vertices++ = (AlGlModelVertex){{p1.x, p1.y}, {n.x, n.y}};
		*vertices++ = (AlGlModelVertex){{p1.x, p1.y}, {-n.x, -n.y}};

	} else {
		*vertices++ = (AlGlModelVertex){{p2.x, p2.y}, {0, 0}};
		*vertices++ = (AlGlModelVertex){{p2.x, p2.y}, {0, 0}};
	}

	PASS()
}

static AlError model_load(AlModel *model, const char *filename)
{
	BEGIN()

	char *filenameCopy = NULL;
	AlModelShape *shape = NULL;

	TRY(al_malloc(&filenameCopy, sizeof(char), strlen(filename) + 1));
	strcpy(filenameCopy, filename);

	TRY(al_model_shape_init(&shape));
	TRY(al_model_shape_load(shape, filename));
	TRY(al_model_set_shape(model, shape));

	model->filename = filenameCopy;

	CATCH(
		al_log_error("Error reading model file: %s", filename);
		free(filenameCopy);
	)
	FINALLY(
		al_model_shape_free(shape);
	)
}

static void model_use(AlModel *model)
{
	if (model->users > 0) {
		model->users++;

	} else {
		model->users = 1;

		if (firstModel) {
			model->next = firstModel;
			firstModel->prev = model;
		}
		firstModel = model;
	}
}

AlError al_model_use_file(AlModel **result, const char *filename)
{
	BEGIN()

	AlModel *newModel = NULL;

	for (AlModel *model = firstModel; model; model = model->next) {
		if (model->filename && !strcmp(model->filename, filename)) {
			*result = model;
			break;
		}
	}

	if (!*result) {
		TRY(model_init(&newModel));
		TRY(model_load(newModel, filename));
		*result = newModel;
	}

	model_use(*result);

	CATCH(
		free(newModel);
	)
	FINALLY()
}

AlError al_model_use_shape(AlModel **result, AlModelShape *shape)
{
	BEGIN()

	AlModel *model = NULL;
	TRY(model_init(&model));
	TRY(al_model_set_shape(model, shape));
	model_use(model);

	*result = model;

	CATCH(
		model_free(model);
	)
	FINALLY()
}

AlError al_model_set_shape(AlModel *model, AlModelShape *shape)
{
	BEGIN()

	Vec3 *colours = NULL;
	int *vertexCounts = NULL;
	AlGlModelVertex *vertices = NULL;

	Vec2 min = {0, 0};
	Vec2 max = {0, 0};

	TRY(al_malloc(&colours, sizeof(Vec3), shape->numPaths));
	TRY(al_malloc(&vertexCounts, sizeof(int), shape->numPaths));

	Vec3 *colour = colours;
	int *vertexCount = vertexCounts;
	AlModelPath *path = shape->paths;
	int totalVertices = 0;
	for (int i = 0; i < shape->numPaths; i++, colour++, vertexCount++, path++) {
		*colour = path->colour;
		*vertexCount = (path->numPoints * 4) - 2;
		totalVertices += *vertexCount;

		for (int j = 0; j < path->numPoints; j++) {
			Vec2 point = path->points[j];

			if (point.x < min.x) min.x = point.x;
			else if (point.x > max.x) max.x = point.x;
			if (point.y < min.y) min.y = point.y;
			else if (point.y > max.y) max.y = point.y;
		}
	}

	TRY(al_malloc(&vertices, sizeof(AlGlModelVertex), totalVertices));

	vertexCount = vertexCounts;
	path = shape->paths;
	AlGlModelVertex *pathVertices = vertices;
	for (int i = 0; i < shape->numPaths; i++, vertexCount++, path++) {
		TRY(model_build_path_vertices(pathVertices, path));
		pathVertices += *vertexCount;
	}

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(AlGlModelVertex) * totalVertices, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	free(model->vertexCounts);
	free(model->colours);

	model->numPaths = shape->numPaths;
	model->vertexCounts = vertexCounts;
	model->colours = colours;
	model->bounds.min = min;
	model->bounds.max = max;

	CATCH(
		al_log_error("Error building GL data from model shape");
		free(colours);
		free(vertexCounts);
	)
	FINALLY(
		free(vertices);
	)
}

void al_model_unuse(AlModel *model)
{
	if (!model)
		return;

	assert(model->users > 0);

	model->users--;
	if (model->users > 0)
		return;

	if (model->prev) {
		model->prev->next = model->next;
	} else {
		firstModel = model->next;
	}

	if (model->next) {
		model->next->prev = model->prev;
	}

	model_free(model);
}

void al_model_get_bounds(AlModel *model, Box *bounds)
{
	*bounds = model->bounds;
}
