/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "albase/gl/opengl.h"
#include "albase/gl/shader.h"
#include "albase/stream.h"

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

AlError algl_shader_init_with_sources(AlGlShader **result, AlGLShaderSource vertexSource, AlGLShaderSource fragmentSource, const char *defines)
{
	BEGIN()

	AlGlShader *shader = NULL;
	TRY(al_malloc(&shader, sizeof(AlGlShader)));

	shader->id = 0;
	shader->vertexShader = 0;
	shader->fragmentShader = 0;

	TRY(build_program(shader, vertexSource, fragmentSource, defines));

	*result = shader;

	CATCH(
		algl_shader_free(shader);
	)
	FINALLY()
}

AlError algl_shader_init_with_streams(AlGlShader **result, AlStream *vertexStream, AlStream *fragmentStream, const char *defines)
{
	BEGIN()

	AlGlShader *shader = NULL;
	char *vertexSource = NULL;
	char *fragmentSource = NULL;

	TRY(al_stream_read_to_string(vertexStream, &vertexSource, NULL));
	TRY(al_stream_read_to_string(fragmentStream, &fragmentSource, NULL));

	TRY(algl_shader_init_with_sources(&shader,
		(AlGLShaderSource){vertexStream->name, vertexSource},
		(AlGLShaderSource){fragmentStream->name, fragmentSource},
		defines));

	*result = shader;

	CATCH({
		algl_shader_free(shader);
	})
	FINALLY({
		al_free(vertexSource);
		al_free(fragmentSource);
	})
}

AlError algl_shader_init_with_files(AlGlShader **result, const char *vertexFilename, const char *fragmentFilename, const char *defines)
{
	BEGIN()

	AlGlShader *shader = NULL;
	AlStream *vertexStream = NULL;
	AlStream *fragmentStream = NULL;

	TRY(al_stream_init_filename(&vertexStream, vertexFilename, AL_OPEN_READ));
	TRY(al_stream_init_filename(&fragmentStream, fragmentFilename, AL_OPEN_READ));

	TRY(algl_shader_init_with_streams(&shader, vertexStream, fragmentStream, defines));

	*result = shader;

	CATCH(
		algl_shader_free(shader);
	)
	FINALLY(
		al_stream_free(vertexStream);
		al_stream_free(fragmentStream);
	)
}

void algl_shader_free(AlGlShader *shader)
{
	if (shader != NULL) {
		glDeleteShader(shader->vertexShader);
		glDeleteShader(shader->fragmentShader);
		glDeleteProgram(shader->id);
		al_free(shader);
	}
}

void algl_uniform_vec2(GLuint uniform, Vec2 v)
{
	glUniform2f(uniform, v.x, v.y);
}

void algl_uniform_vec3(GLuint uniform, Vec3 v)
{
	glUniform3f(uniform, v.x, v.y, v.z);
}

void algl_uniform_vec4(GLuint uniform, Vec4 v)
{
	glUniform4f(uniform, v.x, v.y, v.z, v.w);
}
