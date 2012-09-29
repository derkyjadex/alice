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

bool vec2_is_near_box(Vec2 v, Box box, double distance)
{
	Vec2 boxPoint;

	boxPoint.x = (v.x < box.min.x) ? box.min.x :
		(v.x > box.max.x) ? box.max.x : v.x;
	boxPoint.y = (v.y < box.min.y) ? box.min.y :
		(v.y > box.max.y) ? box.max.y : v.y;

	double distSq = distance * distance;

	return vec2_norm_sq(vec2_subtract(v, boxPoint)) <= distSq;
}

Box box_add_vec2(Box box, Vec2 v)
{
	Box result = {
		{box.min.x + v.x, box.min.y + v.y},
		{box.max.x + v.x, box.max.y + v.y}
	};

	return result;
}

bool box_contains(Box box, Vec2 v)
{
	return v.x >= box.min.x && v.x < box.max.x
		&& v.y >= box.min.y && v.y < box.max.y;
}
