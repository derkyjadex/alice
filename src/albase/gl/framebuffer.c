/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "albase/gl/framebuffer.h"

AlError al_gl_framebuffer_init(AlGlFramebuffer **result)
{
	BEGIN()

	AlGlFramebuffer *framebuffer = NULL;
	TRY(al_malloc(&framebuffer, sizeof(AlGlFramebuffer), 1));

	framebuffer->id = 0;
	framebuffer->colourTex = 0;

	glGenFramebuffers(1, &framebuffer->id);

	glGenTextures(1, &framebuffer->colourTex);
	if (framebuffer->colourTex == 0) {
		THROW(AL_ERROR_GENERIC)
	}

	glBindTexture(GL_TEXTURE_2D, framebuffer->colourTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colourTex, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		al_log_error("Error creating framebuffer: %d", status);
		THROW(AL_ERROR_GENERIC)
	}

	*result = framebuffer;

	CATCH(
		al_gl_framebuffer_free(framebuffer);
	)
	FINALLY(
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	)
}

void al_gl_framebuffer_free(AlGlFramebuffer *framebuffer)
{
	if (framebuffer != NULL) {
		glDeleteFramebuffers(1, &framebuffer->id);
		glDeleteTextures(1, &framebuffer->colourTex);
		free(framebuffer);
	}
}

AlError al_gl_framebuffer_resize(AlGlFramebuffer *framebuffer, int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, framebuffer->colourTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return AL_NO_ERROR;
}
