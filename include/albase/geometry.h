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
} Box2;

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
bool vec2_is_near_box2(Vec2 v, Box2 box, double distance);
Vec2 vec2_floor(Vec2 v);
Vec2 vec2_ceil(Vec2 v);
double vec2_dot(Vec2 a, Vec2 b);
double vec2_cross(Vec2 a, Vec2 b, Vec2 c);
Vec2 vec2_mix(Vec2 a, Vec2 b, double t);

Box2 box2_add_vec2(Box2 box, Vec2 v);
Box2 box2_include_vec2(Box2 box, Vec2 v);
bool box2_contains(Box2 box, Vec2 v);
bool box2_is_valid(Box2 box);
Box2 box2_intersect(Box2 a, Box2 b);
Vec2 box2_size(Box2 box);
Box2 box2_round(Box2 box);
Box2 box2_expand(Box2 box, double a);

#endif
