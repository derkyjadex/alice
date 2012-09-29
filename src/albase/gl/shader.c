/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "albase/gl/opengl.h"
#include "albase/gl/shader.h"
#include "albase/file.h"

static AlError build_program(AlGlShader *shader, const char *vertexSource, const char *fragmentSource);
static AlError compile_shader(GLenum type, const char *source, GLuint *shader);

AlError al_gl_shader_init_with_files(AlGlShader **result, const char *vertexFilename, const char *fragmentFilename)
{
	BEGIN()

	AlGlShader *shader = NULL;
	char *vertexSource = NULL;
	char *fragmentSource = NULL;

	TRY(al_read_file_to_string(vertexFilename, &vertexSource));
	TRY(al_read_file_to_string(fragmentFilename, &fragmentSource));

	TRY(al_gl_shader_init_with_sources(&shader, vertexSource, fragmentSource));

	*result = shader;

	CATCH(
		al_gl_shader_free(shader);
		al_log_error("Error compiling shaders %s and %s", vertexFilename, fragmentFilename);
	)
	FINALLY(
		free(vertexSource);
		free(fragmentSource);
	)
}

AlError al_gl_shader_init_with_sources(AlGlShader **result, const char *vertexSource, const char *fragmentSource)
{
	BEGIN()

	AlGlShader *shader = NULL;
	TRY(al_malloc(&shader, sizeof(AlGlShader), 1));

	shader->id = 0;
	shader->vertexShader = 0;
	shader->fragmentShader = 0;

	TRY(build_program(shader, vertexSource, fragmentSource));

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

static AlError build_program(AlGlShader *shader, const char *vertexSource, const char *fragmentSource)
{
	BEGIN()

	TRY(compile_shader(GL_VERTEX_SHADER, vertexSource, &shader->vertexShader));
	TRY(compile_shader(GL_FRAGMENT_SHADER, fragmentSource, &shader->fragmentShader));

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

		al_log_error("Error linking program:\n%s\n", log);
		THROW(AL_ERROR_GRAPHICS)
	}

	PASS()
}

static AlError compile_shader(GLenum type, const char *source, GLuint *result)
{
	BEGIN()

	const char *sources[2] = {
#ifdef GL_VERSION_2_1
		"#version 120\n",
#else
		"#version 100\n",
#endif
		source
	};

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 2, sources, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		char log[1024];
		GLsizei logLength;

		glGetShaderInfoLog(shader, sizeof(log), &logLength, log);

		al_log_error("Error compiling shader:\n%s\n", log);
		THROW(AL_ERROR_GRAPHICS)
	}

	*result = shader;

	CATCH(
		glDeleteShader(shader);
	)
	FINALLY()
}
