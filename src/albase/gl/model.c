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

	model->bounds = (Box2){{0, 0}, {0, 0}};

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
	AlModelPoint point;
} VertexNode;

static bool triangle_contains(Vec2 t1, Vec2 t2, Vec2 t3, Vec2 p)
{
	return vec2_cross(t1, p, t2) < 0 &&
		   vec2_cross(t2, p, t3) < 0 &&
		   vec2_cross(t3, p, t1) < 0;
}

static void build_vertex_nodes(AlModelPath *path, VertexNode *vertices)
{
	VertexNode *last = vertices;
	*last = (VertexNode){vertices, path->points[path->numPoints - 1]};

	for (int i = 0; i < path->numPoints; i++) {
		if (!path->points[i].onCurve && !last->point.onCurve) {
			Vec2 a = last->point.location;
			Vec2 b = path->points[i].location;

			last->next = last + 1;
			last++;
			*last = (VertexNode){vertices, {
				.location = vec2_scale(vec2_add(a, b), 0.5),
				.onCurve = true
			}};
		}

		if (i != path->numPoints - 1) {
			last->next = last + 1;
			last++;
			*last = (VertexNode){vertices, path->points[i]};
		}
	}
}

static void build_curve_triangles(VertexNode *vertices, AlGlModelVertex *output, int *outputCount)
{
	bool first = true;
	for (VertexNode *node = vertices; first || node != vertices; first = false, node = node->next) {
		if (!node->next->point.onCurve) {
			Vec2 p1 = node->point.location;
			Vec2 p2 = node->next->point.location;
			Vec2 p3 = node->next->next->point.location;

			double cross = vec2_cross(p1, p3, p2);

			float sign;
			if (cross == 0) {
				node->next = node->next->next;
				continue;

			} else if (cross < 0) {
				sign = -1;
				node->next = node->next->next;
			} else {
				sign = 1;
			}

			*output++ = (AlGlModelVertex){{p1.x, p1.y}, {0.0, 0.0, sign}};
			*output++ = (AlGlModelVertex){{p2.x, p2.y}, {0.5, 0.0, sign}};
			*output++ = (AlGlModelVertex){{p3.x, p3.y}, {1.0, 1.0, sign}};
			*outputCount += 3;
		}
	}
}

static void build_inner_triangles(VertexNode *vertices, AlGlModelVertex *output, int *outputCount)
{
	VertexNode *v1 = vertices, *lastSuccess = v1;

	while (true) {
		VertexNode *v3 = v1->next->next;

		if (v3 == v1)
			return;

		Vec2 p1 = v1->point.location;
		Vec2 p2 = v1->next->point.location;
		Vec2 p3 = v3->point.location;
		bool success = false;

		if (vec2_cross(p1, p3, p2) < 0) {
			bool empty = true;

			for (VertexNode *v = v3->next; v != v1; v = v->next) {
				if (triangle_contains(p1, p2, p3, v->point.location)) {
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
				return;
		}
	}

	return;
}

static AlError build_path_vertices(AlModelPath *path, AlGlModelVertex *output, int *outputCount)
{
	BEGIN()

	VertexNode *vertices = NULL;

	if (path->numPoints < 2)
		THROW(AL_ERROR_INVALID_DATA);

	TRY(al_malloc(&vertices, sizeof(VertexNode), path->numPoints * 2));

	build_vertex_nodes(path, vertices);

	VertexNode *first = (vertices->point.onCurve) ?
		vertices : vertices->next;

	*outputCount = 0;

	build_curve_triangles(first, output, outputCount);
	output += *outputCount;

	build_inner_triangles(first, output, outputCount);

	PASS({
		free(vertices);
	})
}

static AlError model_load(AlModel *model, const char *filename)
{
	BEGIN()

	char *filenameCopy = NULL;
	AlStream *stream = NULL;
	AlModelShape *shape = NULL;

	TRY(al_malloc(&filenameCopy, sizeof(char), strlen(filename) + 1));
	strcpy(filenameCopy, filename);

	TRY(al_stream_init_filename(&stream, filename, AL_OPEN_READ));
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

	Box2 bounds = {{0, 0}, {0, 0}};

	TRY(al_malloc(&colours, sizeof(Vec3), shape->numPaths));
	TRY(al_malloc(&vertexCounts, sizeof(int), shape->numPaths));

	AlModelPath **pathPtr = shape->paths;
	Vec3 *colour = colours;
	int maxVertices = 0;

	for (int i = 0; i < shape->numPaths; i++, pathPtr++, colour++) {
		AlModelPath *path = *pathPtr;

		*colour = path->colour;
		maxVertices += path->numPoints * 9 - 6;

		for (int j = 0; j < path->numPoints; j++) {
			bounds = box2_include_vec2(bounds, path->points[j].location);
		}
	}

	TRY(al_malloc(&vertices, sizeof(AlGlModelVertex), maxVertices));

	pathPtr = shape->paths;
	AlGlModelVertex *pathVertices = vertices;
	int *vertexCount = vertexCounts;
	int totalVertices = 0;
	for (int i = 0; i < shape->numPaths; i++, pathPtr++, vertexCount++) {
		TRY(build_path_vertices(*pathPtr, pathVertices, vertexCount));
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

void al_model_get_bounds(AlModel *model, Box2 *bounds)
{
	*bounds = model->bounds;
}
