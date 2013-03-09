/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GEOMETRY_H__
#define __ALBASE_GEOMETRY_H__

#include <stdbool.h>

typedef struct {
	double x, y;
} Vec2;

typedef struct {
	double x, y, z;
} Vec3;

typedef struct {
	double x, y, z, w;
} Vec4;

typedef struct {
	Vec2 min, max;
} Box;

Vec2 vec2_normalise(Vec2 v);
Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_subtract(Vec2 a, Vec2 b);
Vec2 vec2_neg(Vec2 v);
Vec2 vec2_scale(Vec2 v, double a);
double vec2_norm_sq(Vec2 v);
double vec2_norm(Vec2 v);
Vec2 vec2_normal(Vec2 v);
Vec2 vec2_random(void);
bool vec2_equals(Vec2 a, Vec2 b);
bool vec2_near(Vec2 a, Vec2 b, double epsilon);
bool vec2_is_near_box(Vec2 v, Box box, double distance);
Vec2 vec2_floor(Vec2 v);
Vec2 vec2_ceil(Vec2 v);
double vec2_dot(Vec2 a, Vec2 b);
double vec2_cross(Vec2 a, Vec2 b, Vec2 c);

Box box_add_vec2(Box box, Vec2 v);
Box box_include_vec2(Box box, Vec2 v);
bool box_contains(Box box, Vec2 v);
bool box_is_valid(Box box);
Box box_intersect(Box a, Box b);
Vec2 box_size(Box box);
Box box_round(Box box);
Box box_expand(Box box, double a);

#endif
