/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec2 viewportSize;

uniform vec2 min;
uniform vec2 size;
uniform float borderWidth;
uniform vec2 gridSize;
uniform vec2 gridOffset;

attribute vec2 position;

varying vec2 coords;
varying vec2 borderSize;
varying vec2 gridCoords;
varying vec2 gridStep;

void main()
{
	coords = vec2(2) * position - vec2(1);
	borderSize = (vec2(2) * borderWidth) / size;
	if (all(notEqual(gridSize, vec2(0)))) {
		gridCoords = (position * size) / gridSize - (gridOffset / gridSize);
		gridStep = (gridSize - vec2(1)) / gridSize;
	} else {
		gridCoords = vec2(0);
		gridStep = vec2(1);
	}

	vec2 pos = floor(min) + floor(size) * position;
	gl_Position = vec4(vec2(2) * pos / viewportSize - vec2(1), 0, 1);
}
