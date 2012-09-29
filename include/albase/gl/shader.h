/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_SHADER_H__
#define __ALBASE_GL_SHADER_H__

#include "albase/common.h"
#include "albase/gl/opengl.h"

#define AL_GET_GL_UNIFORM(shaderVar, name) shaderVar.name = glGetUniformLocation(shaderVar.shader->id, #name)

#define AL_GET_GL_ATTRIB(shaderVar, name) shaderVar.name = glGetAttribLocation(shaderVar.shader->id, #name)

typedef struct AlGlShader {
	GLuint id;
	GLuint vertexShader;
	GLuint fragmentShader;
} AlGlShader;

AlError al_gl_shader_init_with_files(AlGlShader **shader, const char *vertexFilename, const char *fragmentFilename);
AlError al_gl_shader_init_with_sources(AlGlShader **shader, const char *vertexSource, const char *fragmentSource);
void al_gl_shader_free(AlGlShader *shader);

#endif
