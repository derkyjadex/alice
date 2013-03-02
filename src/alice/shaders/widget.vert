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
const float _gridWidth = 1.0;

attribute vec2 position;

varying vec2 borderCoords;
varying vec2 borderStep;
varying vec2 gridCoords;
varying vec2 gridStep;

void main()
{
	vec2 _min = floor(min);
	vec2 _size = floor(size);
	float _borderWidth = floor(borderWidth);
	vec2 _gridSize = floor(gridSize);
	vec2 _gridOffset = floor(gridOffset);

	borderCoords = 2.0 * position - 1.0;
	borderStep = (2.0 * _borderWidth) / _size;

	if (all(notEqual(gridSize, vec2(0)))) {
		gridCoords = (_size * position - _gridOffset - _borderWidth) / _gridSize;
		gridStep = _gridWidth / _gridSize;

	} else {
		gridCoords = vec2(0);
		gridStep = vec2(0);
	}

	vec2 pos = _min + _size * position;
	gl_Position = vec4(2.0 * pos / viewportSize - 1.0, 0, 1);
}
