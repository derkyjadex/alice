/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "albase/gl/opengl.h"
#include "albase/gl/shader.h"
#include "albase/stream.h"

static AlError build_program(AlGlShader *shader, AlGLShaderSource vertexSource, AlGLShaderSource fragmentSource, const char *defines);
static AlError compile_shader(GLenum type, AlGLShaderSource source, const char *defines, GLuint *shader);

AlError al_gl_shader_init_with_files(AlGlShader **result, const char *vertexFilename, const char *fragmentFilename, const char *defines)
{
	BEGIN()

	AlGlShader *shader = NULL;
	char *vertexSource = NULL;
	char *fragmentSource = NULL;

	TRY(al_read_file_to_string(vertexFilename, &vertexSource));
	TRY(al_read_file_to_string(fragmentFilename, &fragmentSource));

	TRY(al_gl_shader_init_with_sources(&shader,
		(AlGLShaderSource){vertexFilename, vertexSource},
		(AlGLShaderSource){fragmentFilename, fragmentSource},
		defines));

	*result = shader;

	CATCH(
		al_gl_shader_free(shader);
	)
	FINALLY(
		free(vertexSource);
		free(fragmentSource);
	)
}

AlError al_gl_shader_init_with_sources(AlGlShader **result, AlGLShaderSource vertexSource, AlGLShaderSource fragmentSource, const char *defines)
{
	BEGIN()

	AlGlShader *shader = NULL;
	TRY(al_malloc(&shader, sizeof(AlGlShader), 1));

	shader->id = 0;
	shader->vertexShader = 0;
	shader->fragmentShader = 0;

	TRY(build_program(shader, vertexSource, fragmentSource, defines));

	*result = shader;

	CATCH(
		al_gl_shader_free(shader);
	)
	FINALLY()
}

void al_gl_shader_free(AlGlShader *shader)
{
	if (shader != NULL) {
		glDeleteShader(shader->vertexShader);
		glDeleteShader(shader->fragmentShader);
		glDeleteProgram(shader->id);
		free(shader);
	}
}

static AlError build_program(AlGlShader *shader, AlGLShaderSource vertexSource, AlGLShaderSource fragmentSource, const char *defines)
{
	BEGIN()

	TRY(compile_shader(GL_VERTEX_SHADER, vertexSource, defines, &shader->vertexShader));
	TRY(compile_shader(GL_FRAGMENT_SHADER, fragmentSource, defines, &shader->fragmentShader));

	shader->id = glCreateProgram();
	glAttachShader(shader->id, shader->vertexShader);
	glAttachShader(shader->id, shader->fragmentShader);

	glLinkProgram(shader->id);

	GLint status;
	glGetProgramiv(shader->id, GL_LINK_STATUS, &status);

	if (status == GL_FALSE) {
		char log[1024];
		GLsizei logLength;

		glGetProgramInfoLog(shader->id, sizeof(log), &logLength, log);

		al_log_error("Error linking program (%s + %s):\n%s\n",
			vertexSource.name, fragmentSource.name, log);
		THROW(AL_ERROR_GRAPHICS)
	}

	PASS()
}

static AlError compile_shader(GLenum type, AlGLShaderSource source, const char *defines, GLuint *result)
{
	BEGIN()

	const char *sources[] = {
#ifdef GL_VERSION_2_1
		"#version 120\n",
#else
		"#version 100\n",
#endif
		(!defines) ? "" : defines,
		"\n",
		source.source
	};

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 4, sources, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		char log[1024];
		GLsizei logLength;

		glGetShaderInfoLog(shader, sizeof(log), &logLength, log);

		al_log_error("Error compiling shader (%s):\n%s\n", source.name, log);
		THROW(AL_ERROR_GRAPHICS)
	}

	*result = shader;

	CATCH(
		glDeleteShader(shader);
	)
	FINALLY()
}

void al_gl_uniform_vec2(GLuint uniform, Vec2 v)
{
	glUniform2f(uniform, v.x, v.y);
}

void al_gl_uniform_vec3(GLuint uniform, Vec3 v)
{
	glUniform3f(uniform, v.x, v.y, v.z);
}

void al_gl_uniform_vec4(GLuint uniform, Vec4 v)
{
	glUniform4f(uniform, v.x, v.y, v.z, v.w);
}
