/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec3 colour;

varying vec3 p;

void main()
{
	float s = p.x * p.x - p.y;
	float a = step(0.0, p.z * s);

	gl_FragColor = vec4(colour, a);
}