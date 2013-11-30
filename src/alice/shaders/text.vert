/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec2 viewportSize;

uniform vec2 minCoord;
uniform vec2 size;
uniform vec2 charMin;
uniform vec2 charSize;

attribute vec2 position;

varying vec2 texCoords;

void main()
{
	texCoords = charMin + charSize * vec2(position.x, 1.0 - position.y);

	vec2 pos = minCoord + size * position;
	gl_Position = vec4(vec2(2) * pos / viewportSize - vec2(1), 0, 1);
}
