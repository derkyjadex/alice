/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec2 viewportSize;

uniform vec2 translate;
uniform float scale;

attribute vec2 position;
attribute vec3 param;

varying vec3 p;

void main()
{
	vec2 pos = translate + scale * position;

	gl_Position = vec4(vec2(2) * pos / viewportSize - vec2(1), 0, 1) ;
	p = param;
}
