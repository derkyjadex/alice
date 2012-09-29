/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform sampler2D font;
uniform vec3 colour;
uniform vec2 edge;

varying vec2 texCoords;

void main()
{
	float alpha = smoothstep(edge.x, edge.y, texture2D(font, texCoords).r);
	gl_FragColor = vec4(colour, alpha);
}
