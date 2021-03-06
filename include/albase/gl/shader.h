/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __ALBASE_GL_SHADER_H__
#define __ALBASE_GL_SHADER_H__

#include "albase/common.h"
#include "albase/gl/opengl.h"
#include "albase/geometry.h"
#include "albase/stream.h"

#define AL_VERT_SHADER_VAR(name) shaders_##name##_vert
#define AL_FRAG_SHADER_VAR(name) shaders_##name##_frag
#define AL_VERT_SHADER_DECLARE(name) \
	extern const char AL_VERT_SHADER_VAR(name)[];
#define AL_FRAG_SHADER_DECLARE(name) \
	extern const char AL_FRAG_SHADER_VAR(name)[];

#define AL_SHADER_DECLARE(name) \
	AL_VERT_SHADER_DECLARE(name) \
	AL_FRAG_SHADER_DECLARE(name)

#define AL_VERT_SHADER(name) (AlGLShaderSource){#name".vert", AL_VERT_SHADER_VAR(name)}
#define AL_FRAG_SHADER(name) (AlGLShaderSource){#name".frag", AL_FRAG_SHADER_VAR(name)}

typedef struct {
	const char *name;
	const char *source;
} AlGLShaderSource;


#define ALGL_GET_UNIFORM(shaderVar, name) shaderVar.name = glGetUniformLocation(shaderVar.shader->id, #name)
#define ALGL_GET_ATTRIB(shaderVar, name) shaderVar.name = glGetAttribLocation(shaderVar.shader->id, #name)

typedef struct AlGlShader {
	GLuint id;
	GLuint vertexShader;
	GLuint fragmentShader;
} AlGlShader;

AlError algl_shader_init_with_sources(AlGlShader **result, AlGLShaderSource vertexSource, AlGLShaderSource fragmentSource, const char *defines);
AlError algl_shader_init_with_streams(AlGlShader **shader, AlStream *vertexStream, AlStream *fragmentStream, const char *defines);
AlError algl_shader_init_with_files(AlGlShader **shader, const char *vertexFilename, const char *fragmentFilename, const char *defines);
void algl_shader_free(AlGlShader *shader);

void algl_uniform_vec2(GLuint uniform, Vec2 v);
void algl_uniform_vec3(GLuint uniform, Vec3 v);
void algl_uniform_vec4(GLuint uniform, Vec4 v);

#endif
