/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec4 fillColour;
uniform vec4 borderColour;
uniform vec4 gridColour;

varying vec2 borderCoords;
varying vec2 borderStep;
varying vec2 gridCoords;
varying vec2 gridStep;

void main()
{
	vec2 grid = step(gridStep, fract(gridCoords));
	vec2 border = step(borderStep, abs(borderCoords));

	vec4 innerColour = mix(gridColour, fillColour, min(grid.x, grid.y));
	gl_FragColor = mix(innerColour, borderColour, max(border.x, border.y));
}
