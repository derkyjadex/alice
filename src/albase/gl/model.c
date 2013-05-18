/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "albase/gl/model.h"
#include "albase/model_shape.h"
#include "../model_shape_internal.h"

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

typedef struct VertexNode {
	struct VertexNode *next;
	int i;
} VertexNode;

static bool triangle_contains(Vec2 t1, Vec2 t2, Vec2 t3, Vec2 p)
{
	return vec2_cross(t1, p, t2) < 0 &&
		   vec2_cross(t2, p, t3) < 0 &&
		   vec2_cross(t3, p, t1) < 0;
}

static AlError model_build_curve_triangles(AlModelPath *path, VertexNode *nodes, AlGlModelVertex *output, int *outputCount)
{
	BEGIN()

	int n = path->numPoints;
	Vec2 *points = path->points;

	VertexNode *lastNode = nodes;
	*lastNode = (VertexNode){nodes, 0};

	for (int i = 0; i < n - 1; i += 2) {
		bool last = i == n - 2;

		Vec2 p1 = points[i + 0];
		Vec2 p2 = points[i + 1];
		Vec2 p3 = points[last ? 0 : i + 2];

		float sign = (vec2_cross(p1, p3, p2) < 0) ? -1 : 1;

		*output++ = (AlGlModelVertex){{p1.x, p1.y}, {0.0, 0.0, sign}};
		*output++ = (AlGlModelVertex){{p2.x, p2.y}, {0.5, 0.0, sign}};
		*output++ = (AlGlModelVertex){{p3.x, p3.y}, {1.0, 1.0, sign}};
		*outputCount += 3;

		if (sign > 0) {
			lastNode->next = lastNode + 1;
			lastNode++;
			*lastNode = (VertexNode){nodes, i + 1};
		}

		if (!last) {
			lastNode->next = lastNode + 1;
			lastNode++;
			*lastNode = (VertexNode){nodes, i + 2};
		}
	}

	PASS()
}

static AlError model_build_inner_triangles(AlModelPath *path, VertexNode *vertices, AlGlModelVertex *output, int *outputCount)
{
	BEGIN()

	Vec2 *points = path->points;
	VertexNode *v1 = vertices, *lastSuccess = v1;

	while (true) {
		VertexNode *v3 = v1->next->next;

		if (v3 == v1)
			RETURN()

		Vec2 p1 = points[v1->i];
		Vec2 p2 = points[v1->next->i];
		Vec2 p3	= points[v3->i];
		bool success = false;

		if (vec2_cross(p1, p3, p2) < 0) {
			bool empty = true;

			for (VertexNode *v = v3->next; v != v1; v = v->next) {
				if (triangle_contains(p1, p2, p3, points[v->i])) {
					empty = false;
					break;
				}
			}

			if (empty) {
				*output++ = (AlGlModelVertex){{p1.x, p1.y}, {0.0, 1.0, -1}};
				*output++ = (AlGlModelVertex){{p2.x, p2.y}, {0.0, 1.0, -1}};
				*output++ = (AlGlModelVertex){{p3.x, p3.y}, {0.0, 1.0, -1}};
				*outputCount += 3;

				v1->next = v3;
				success = true;
				lastSuccess = v1;
			}
		}

		if (!success) {
			v1 = v1->next;

			if (v1 == lastSuccess)
				RETURN()
		}
	}

	PASS()
}

static AlError model_build_path_vertices(AlModelPath *path, AlGlModelVertex *output, int *outputCount)
{
	BEGIN()

	int n = path->numPoints;
	VertexNode *nodes = NULL;

	if (n < 2)
		THROW(AL_ERROR_INVALID_DATA)

	*outputCount = 0;

	TRY(al_malloc(&nodes, sizeof(VertexNode), n));

	TRY(model_build_curve_triangles(path, nodes, output, outputCount));
	output += *outputCount;

	TRY(model_build_inner_triangles(path, nodes, output, outputCount));

	PASS(
		free(nodes);
	)
}

static AlError model_load(AlModel *model, const char *filename)
{
	BEGIN()

	char *filenameCopy = NULL;
	AlStream *stream = NULL;
	AlModelShape *shape = NULL;

	TRY(al_malloc(&filenameCopy, sizeof(char), strlen(filename) + 1));
	strcpy(filenameCopy, filename);

	TRY(al_stream_init_file(&stream, filename, AL_OPEN_READ));
	TRY(al_model_shape_init(&shape));
	TRY(al_model_shape_load(shape, stream));
	TRY(al_model_set_shape(model, shape));

	model->filename = filenameCopy;

	CATCH(
		al_log_error("Error reading model file: %s", filename);
		free(filenameCopy);
	)
	FINALLY(
		al_stream_free(stream);
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

	Box bounds = {{0, 0}, {0, 0}};

	TRY(al_malloc(&colours, sizeof(Vec3), shape->numPaths));
	TRY(al_malloc(&vertexCounts, sizeof(int), shape->numPaths));

	AlModelPath **pathPtr = shape->paths;
	Vec3 *colour = colours;
	int maxVertices = 0;

	for (int i = 0; i < shape->numPaths; i++, pathPtr++, colour++) {
		AlModelPath *path = *pathPtr;

		*colour = path->colour;
		maxVertices += ceil(path->numPoints / 2.0) * 9 - 6;

		for (int j = 0; j < path->numPoints; j++) {
			bounds = box_include_vec2(bounds, path->points[j]);
		}
	}

	TRY(al_malloc(&vertices, sizeof(AlGlModelVertex), maxVertices));

	pathPtr = shape->paths;
	AlGlModelVertex *pathVertices = vertices;
	int *vertexCount = vertexCounts;
	int totalVertices = 0;
	for (int i = 0; i < shape->numPaths; i++, pathPtr++, vertexCount++) {
		TRY(model_build_path_vertices(*pathPtr, pathVertices, vertexCount));
		pathVertices += *vertexCount;
		totalVertices += *vertexCount;
	}

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(AlGlModelVertex) * totalVertices, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	free(model->colours);
	free(model->vertexCounts);

	model->numPaths = shape->numPaths;
	model->colours = colours;
	model->vertexCounts = vertexCounts;
	model->bounds = bounds;

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
