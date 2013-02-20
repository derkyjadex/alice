/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "widget_graphics.h"
#include "albase/model.h"
#include "albase/gl/opengl.h"
#include "albase/gl/shader.h"
#include "albase/gl/model.h"
#include "albase/gl/texture.h"
#include "albase/gl/system.h"
#include "shaders.h"
#include "images.h"
#include "graphics_text.h"

static float viewportSize[2];

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint min;
	GLuint size;
	GLuint borderWidth;
	GLuint gridSize;
	GLuint gridOffset;
	GLuint fillColour;
	GLuint borderColour;
	GLuint gridColour;
	GLuint position;
} widgetShader;

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint translate;
	GLuint scale;
	GLuint colour;
	GLuint position;
	GLuint param;
} modelShader;

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint min;
	GLuint size;
	GLuint charMin;
	GLuint charSize;
	GLuint font;
	GLuint colour;
	GLuint edge;
	GLuint position;

	AlGlTexture *texture;
} textShader;

static struct {
	int numCharsW;
	int numCharsH;
	float charWidth;
	float xAdvance;
	float edgeCenter;
	float edgeSpread;
} fontInfo;

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint location;
	GLuint size;
	GLuint image;
	GLuint position;

	AlGlTexture *texture;
} cursorShader;

static struct {
	Vec2 textureSize;
	Vec2 midPoint;
} cursorInfo;

static void free_shaders()
{
	al_gl_shader_free(widgetShader.shader);
	widgetShader.shader = NULL;
	al_gl_shader_free(modelShader.shader);
	modelShader.shader = NULL;

	al_gl_shader_free(textShader.shader);
	textShader.shader = NULL;
	al_gl_texture_free(textShader.texture);
	textShader.texture = NULL;

	al_gl_shader_free(cursorShader.shader);
	cursorShader.shader = NULL;
	al_gl_texture_free(cursorShader.texture);
	cursorShader.texture = NULL;
}

static AlError init_shaders()
{
	BEGIN()

	widgetShader.shader = NULL;
	modelShader.shader = NULL;
	textShader.shader = NULL;
	textShader.texture = NULL;
	cursorShader.shader = NULL;
	cursorShader.texture = NULL;

	TRY(al_gl_shader_init_with_sources(&widgetShader.shader, AL_VERT_SHADER(widget), AL_FRAG_SHADER(widget)));
	AL_GET_GL_UNIFORM(widgetShader, viewportSize);
	AL_GET_GL_UNIFORM(widgetShader, min);
	AL_GET_GL_UNIFORM(widgetShader, size);
	AL_GET_GL_UNIFORM(widgetShader, borderWidth);
	AL_GET_GL_UNIFORM(widgetShader, gridSize);
	AL_GET_GL_UNIFORM(widgetShader, gridOffset);
	AL_GET_GL_UNIFORM(widgetShader, fillColour);
	AL_GET_GL_UNIFORM(widgetShader, borderColour);
	AL_GET_GL_UNIFORM(widgetShader, gridColour);
	AL_GET_GL_ATTRIB(widgetShader, position);

	TRY(al_gl_shader_init_with_sources(&modelShader.shader, AL_VERT_SHADER(model), AL_FRAG_SHADER(model)));
	AL_GET_GL_UNIFORM(modelShader, viewportSize);
	AL_GET_GL_UNIFORM(modelShader, translate);
	AL_GET_GL_UNIFORM(modelShader, scale);
	AL_GET_GL_UNIFORM(modelShader, colour);
	AL_GET_GL_ATTRIB(modelShader, position);
	AL_GET_GL_ATTRIB(modelShader, param);

	TRY(al_gl_shader_init_with_sources(&textShader.shader, AL_VERT_SHADER(text), AL_FRAG_SHADER(text)));
	AL_GET_GL_UNIFORM(textShader, viewportSize);
	AL_GET_GL_UNIFORM(textShader, min);
	AL_GET_GL_UNIFORM(textShader, size);
	AL_GET_GL_UNIFORM(textShader, charMin);
	AL_GET_GL_UNIFORM(textShader, charSize);
	AL_GET_GL_UNIFORM(textShader, font);
	AL_GET_GL_UNIFORM(textShader, colour);
	AL_GET_GL_UNIFORM(textShader, edge);
	AL_GET_GL_ATTRIB(textShader, position);

	TRY(al_gl_texture_init(&textShader.texture));
	TRY(al_gl_texture_load_from_buffer(textShader.texture, images_font_png, images_font_png_size));

	fontInfo.numCharsW = 16;
	fontInfo.numCharsH = 16;
	fontInfo.charWidth = 0.5;
	fontInfo.xAdvance = 100.0 / 256.0;
	fontInfo.edgeCenter = 0.38;
	fontInfo.edgeSpread = 4.0;

	TRY(al_gl_shader_init_with_sources(&cursorShader.shader, AL_VERT_SHADER(cursor), AL_FRAG_SHADER(cursor)));
	AL_GET_GL_UNIFORM(cursorShader, viewportSize);
	AL_GET_GL_UNIFORM(cursorShader, location);
	AL_GET_GL_UNIFORM(cursorShader, size);
	AL_GET_GL_UNIFORM(cursorShader, image);
	AL_GET_GL_ATTRIB(cursorShader, position);

	TRY(al_gl_texture_init(&cursorShader.texture));
	TRY(al_gl_texture_load_from_buffer(cursorShader.texture, images_cursor_png, images_cursor_png_size));

	cursorInfo.textureSize = (Vec2){32, 32};
	cursorInfo.midPoint = (Vec2){16, 16};

	CATCH(
		free_shaders();
	)
	FINALLY()
}

AlError widget_graphics_system_init()
{
	BEGIN()

	TRY(al_gl_system_init());

	Vec2 size = al_gl_system_screen_size();
	viewportSize[0] = size.x;
	viewportSize[1] = size.y;

	glViewport(0, 0, size.x, size.y);

	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	TRY(init_shaders());

	CATCH(
		widget_graphics_system_free();
	)
	FINALLY()
}

void widget_graphics_system_free()
{
	free_shaders();
	al_gl_system_free();
}

Vec2 widget_graphics_screen_size()
{
	return al_gl_system_screen_size();
}

static void render_model(AlModel *model, Vec2 location, double scale)
{
	glUseProgram(modelShader.shader->id);
	glUniform2fv(modelShader.viewportSize, 1, viewportSize);
	glUniform2f(modelShader.translate, location.x, location.y);
	glUniform1f(modelShader.scale, scale);

	glEnableVertexAttribArray(modelShader.position);
	glEnableVertexAttribArray(modelShader.param);

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer);
	glVertexAttribPointer(modelShader.position, 2, GL_FLOAT, GL_FALSE, sizeof(AlGlModelVertex), (void *)offsetof(AlGlModelVertex, position));
	glVertexAttribPointer(modelShader.param, 3, GL_FLOAT, GL_FALSE, sizeof(AlGlModelVertex), (void *)offsetof(AlGlModelVertex, param));

	int start = 0;
	for (int i = 0; i < model->numPaths; i++) {
		Vec3 colour = model->colours[i];
		glUniform3f(modelShader.colour, colour.x, colour.y, colour.z);
		glDrawArrays(GL_TRIANGLES, start, model->vertexCounts[i]);
		start += model->vertexCounts[i];
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(modelShader.position);
	glDisableVertexAttribArray(modelShader.param);
}

static void render_text(const char *text, Vec3 colour, Vec2 location, double size)
{
	float charWidth = fontInfo.charWidth * size;
	double edgeSpread = fontInfo.edgeSpread / size;

	glUseProgram(textShader.shader->id);
	glUniform2fv(textShader.viewportSize, 1, viewportSize);
	glUniform2f(textShader.size, charWidth, size);
	glUniform2f(textShader.charSize, 1.0 / fontInfo.numCharsW, 1.0 / fontInfo.numCharsH);
	glUniform3f(textShader.colour, colour.x, colour.y, colour.z);
	glUniform2f(textShader.edge, fontInfo.edgeCenter - edgeSpread, fontInfo.edgeCenter + edgeSpread);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textShader.texture->id);
	glUniform1i(textShader.font, 0);

	float vertices[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

	glEnableVertexAttribArray(widgetShader.position);
	glVertexAttribPointer(widgetShader.position, 2, GL_FLOAT, GL_FALSE, 0, vertices);

	struct TextReadState state;
	graphics_text_read_init(&state, text);
	uint8_t c;
	while ((c = graphics_text_read_next(&state))) {
		float x = (float)(c % fontInfo.numCharsW) / fontInfo.numCharsW;
		float y = (float)(c / fontInfo.numCharsW) / fontInfo.numCharsH;

		glUniform2f(textShader.min, location.x, location.y);
		glUniform2f(textShader.charMin, x, y);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		location.x += fontInfo.xAdvance * size;
	}
}

static void render_widget(AlWidget *widget, Vec2 translate, Box scissor)
{
	Vec2 location = vec2_add(widget->location, translate);
	Box bounds = box_add_vec2(widget->bounds, location);

	scissor = box_round(box_intersect(scissor, bounds));

	if (box_is_valid(scissor)) {
		Vec2 scissorSize = box_size(scissor);
		Vec2 size = box_size(bounds);
		Vec4 fillColour = widget->fillColour;
		Vec4 borderColour = widget->border.colour;

		glUseProgram(widgetShader.shader->id);
		glUniform2fv(widgetShader.viewportSize, 1, viewportSize);
		glUniform2f(widgetShader.min, bounds.min.x, bounds.min.y);
		glUniform2f(widgetShader.size, size.x, size.y);
		glUniform1f(widgetShader.borderWidth, widget->border.width);
		glUniform2f(widgetShader.gridSize, widget->grid.size.x, widget->grid.size.y);
		glUniform2f(widgetShader.gridOffset, widget->grid.offset.x, widget->grid.offset.y);
		glUniform4f(widgetShader.fillColour, fillColour.x, fillColour.y, fillColour.z, fillColour.w);
		glUniform4f(widgetShader.borderColour, borderColour.x, borderColour.y, borderColour.z, borderColour.w);
		glUniform3f(widgetShader.gridColour, widget->grid.colour.x, widget->grid.colour.y, widget->grid.colour.z);

		const float vertices[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

		glEnableVertexAttribArray(widgetShader.position);
		glVertexAttribPointer(widgetShader.position, 2, GL_FLOAT, GL_FALSE, 0, vertices);

		glScissor(scissor.min.x, scissor.min.y, scissorSize.x, scissorSize.y);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		if (widget->model.model) {
			render_model(widget->model.model, vec2_add(widget->model.location, location) , widget->model.scale);
		}

		if (widget->text.value) {
			render_text(widget->text.value, widget->text.colour, vec2_add(widget->text.location, location), widget->text.size);
		}

		FOR_EACH_WIDGET(child, widget) {
			render_widget(child, location, scissor);
		}
	}

	widget->valid = true;
}

static void render_cursor(Vec2 location)
{
	location = vec2_subtract(location, cursorInfo.midPoint);

	glUseProgram(cursorShader.shader->id);
	glUniform2fv(cursorShader.viewportSize, 1, viewportSize);
	glUniform2f(cursorShader.location, location.x, location.y);
	glUniform2f(cursorShader.size, cursorInfo.textureSize.x, cursorInfo.textureSize.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cursorShader.texture->id);
	glUniform1i(cursorShader.image, 0);

	float vertices[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

	glEnableVertexAttribArray(widgetShader.position);
	glVertexAttribPointer(widgetShader.position, 2, GL_FLOAT, GL_FALSE, 0, vertices);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void widget_graphics_render(AlWidget *root, bool renderCursor, Vec2 cursorLocation)
{
	if (!root->valid) {
		render_widget(root, (Vec2){0, 0}, (Box){{0, 0}, {viewportSize[0], viewportSize[1]}});

		if (renderCursor) {
			render_cursor(cursorLocation);
		}

		al_gl_system_swap_buffers();
	}
}
