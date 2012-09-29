/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

uniform sampler2D image;

varying vec2 texCoords;

void main()
{
	gl_FragColor = texture2D(image, texCoords);
}
