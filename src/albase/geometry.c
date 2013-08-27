/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <math.h>
#include <stdlib.h>

#include "albase/geometry.h"
#include "albase/common.h"

Vec2 vec2_normalise(Vec2 v)
{
	double norm = vec2_norm(v);
	Vec2 result = (norm == 0) ? (Vec2){0, 0} : (Vec2){v.x / norm, v.y / norm};

	return result;
}

Vec2 vec2_add(Vec2 a, Vec2 b)
{
	Vec2 result = {a.x + b.x, a.y + b.y};

	return result;
}

Vec2 vec2_subtract(Vec2 a, Vec2 b)
{
	Vec2 result = {a.x - b.x, a.y - b.y};

	return result;
}

Vec2 vec2_neg(Vec2 v)
{
	return (Vec2){-v.x, -v.y};
}

Vec2 vec2_scale(Vec2 v, double a)
{
	Vec2 result = {v.x * a, v.y * a};

	return result;
}

double vec2_norm_sq(Vec2 v)
{
	return v.x * v.x + v.y * v.y;
}

double vec2_norm(Vec2 v)
{
	return sqrt(vec2_norm_sq(v));
}

Vec2 vec2_normal(Vec2 v)
{
	return (Vec2){-v.y, v.x};
}

Vec2 vec2_random()
{
	double angle = 2 * M_PI * (double)(random() % 1000) / 1000;

	return (Vec2){cos(angle), sin(angle)};
}

bool vec2_equals(Vec2 a, Vec2 b)
{
	return a.x = b.x && a.y == b.y;
}

bool vec2_near(Vec2 a, Vec2 b, double epsilon)
{
	return fabs(a.x - b.x) < epsilon && fabs(a.y - b.y) < epsilon;
}

bool vec2_is_near_box2(Vec2 v, Box2 box, double distance)
{
	Vec2 boxPoint;

	boxPoint.x = (v.x < box.min.x) ? box.min.x :
		(v.x > box.max.x) ? box.max.x : v.x;
	boxPoint.y = (v.y < box.min.y) ? box.min.y :
		(v.y > box.max.y) ? box.max.y : v.y;

	double distSq = distance * distance;

	return vec2_norm_sq(vec2_subtract(v, boxPoint)) <= distSq;
}

Vec2 vec2_floor(Vec2 v)
{
	return (Vec2){floor(v.x), floor(v.y)};
}

Vec2 vec2_ceil(Vec2 v)
{
	return (Vec2){ceil(v.x), ceil(v.y)};
}

double vec2_dot(Vec2 a, Vec2 b)
{
	return a.x * b.x + a.y + b.y;
}

double vec2_cross(Vec2 a, Vec2 b, Vec2 c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

Box2 box2_add_vec2(Box2 box, Vec2 v)
{
	return (Box2){
		{box.min.x + v.x, box.min.y + v.y},
		{box.max.x + v.x, box.max.y + v.y}
	};
}

Box2 box2_include_vec2(Box2 box, Vec2 v)
{
	Box2 result;

	if (v.x < box.min.x) {
		result.min.x = v.x;
		result.max.x = box.max.x;

	} else if (v.x > box.max.x) {
		result.min.x = box.min.x;
		result.max.x = v.x;

	} else {
		result.min.x = box.min.x;
		result.max.x = box.max.x;
	}

	if (v.y < box.min.y) {
		result.min.y = v.y;
		result.max.y = box.max.y;

	} else if (v.y > box.max.y) {
		result.min.y = box.min.y;
		result.max.y = v.y;

	} else {
		result.min.y = box.min.y;
		result.max.y = box.max.y;
	}

	return result;
}

bool box2_contains(Box2 box, Vec2 v)
{
	return v.x >= box.min.x && v.x < box.max.x
		&& v.y >= box.min.y && v.y < box.max.y;
}

bool box2_is_valid(Box2 box)
{
	return box.min.x <= box.max.x && box.min.y <= box.max.y;
}

Box2 box2_intersect(Box2 a, Box2 b)
{
	return (Box2){
		{
			(a.min.x > b.min.x) ? a.min.x : b.min.x,
			(a.min.y > b.min.y) ? a.min.y : b.min.y
		},
		{
			(a.max.x < b.max.x) ? a.max.x : b.max.x,
			(a.max.y < b.max.y) ? a.max.y : b.max.y
		}
	};
}

Vec2 box2_size(Box2 box)
{
	return vec2_subtract(box.max, box.min);
}

Box2 box2_round(Box2 box)
{
	return (Box2){vec2_floor(box.min), vec2_ceil(box.max)};
}

Box2 box2_expand(Box2 box, double a)
{
	return (Box2){
		{box.min.x - a, box.min.y - a},
		{box.max.x + a, box.max.y + a}
	};
}
