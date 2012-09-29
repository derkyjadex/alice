/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform vec3 colour;

void main()
{
	gl_FragColor = vec4(colour, 1);
}