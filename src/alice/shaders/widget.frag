/*
 * Copyright (c) 2011-2013 James Deery
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
	vec4 innerColour;

#ifdef WITH_GRID
	vec2 grid = step(gridStep, fract(gridCoords));
	innerColour = mix(gridColour, fillColour, min(grid.x, grid.y));
#else
	innerColour = fillColour;
#endif

#ifdef WITH_BORDER
	vec2 border = step(borderStep, abs(borderCoords));
	gl_FragColor = mix(innerColour, borderColour, max(border.x, border.y));
#else
	gl_FragColor = innerColour;
#endif
}
