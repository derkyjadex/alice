/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec2 viewportSize;

uniform vec2 translate;
uniform float scale;

uniform float width;

attribute vec2 position;
attribute vec2 normal;

void main()
{
	vec2 pos = translate + scale * position + width * 0.5 * normal;
	gl_Position = vec4(vec2(2) * pos / viewportSize - vec2(1), 0, 1) ;
}
