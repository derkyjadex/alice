/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec4 fillColour;
uniform vec4 borderColour;
uniform vec3 gridColour;

varying vec2 coords;
varying vec2 borderSize;
varying vec2 gridCoords;
varying vec2 gridStep;

void main()
{
	vec2 borderOrFill = step(vec2(1) - borderSize, abs(coords));

	if (any(equal(borderOrFill, vec2(1)))) {
		gl_FragColor = borderColour;

	} else {
		vec2 gridOrFill = step(gridStep, fract(gridCoords));

		if (any(equal(gridOrFill, vec2(1)))) {
			gl_FragColor = vec4(gridColour, 1.0);
		} else {
			gl_FragColor = fillColour;
		}
	}
}
