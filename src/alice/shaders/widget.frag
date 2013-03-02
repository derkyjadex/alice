/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec4 fillColour;
uniform vec4 borderColour;
uniform vec3 gridColour;

varying vec2 borderCoords;
varying vec2 borderStep;
varying vec2 gridCoords;
varying vec2 gridStep;

void main()
{
	vec2 grid = step(gridStep, fract(gridCoords));
	vec2 border = step(borderStep, 1.0 - abs(borderCoords));

	vec4 innerColour = mix(vec4(gridColour, 1), fillColour, min(grid.x, grid.y));
	gl_FragColor = mix(borderColour, innerColour, min(border.x, border.y));
}
