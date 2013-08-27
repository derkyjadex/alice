/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "graphics.h"
#include "albase/model.h"
#include "albase/gl/opengl.h"
#include "albase/gl/shader.h"
#include "albase/gl/model.h"
#include "albase/gl/texture.h"
#include "albase/gl/system.h"
#include "shaders.h"
#include "images.h"
#include "graphics_text.h"
#include "widget_internal.h"

static Vec2 viewportSize;

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint min;
	GLuint size;
	GLuint fillColour;
	GLuint position;
} plainWidgetShader;

static struct {
	AlGlShader *shader;
	GLuint viewportSize;
	GLuint min;
	GLuint size;
	GLuint borderWidth;
	GLuint fillColour;
	GLuint borderColour;
	GLuint position;
} borderWidgetShader;

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
} gridBorderWidgetShader;

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

static GLuint plainVertices;

static void free_shaders()
{
	algl_shader_free(plainWidgetShader.shader);
	plainWidgetShader.shader = NULL;
	algl_shader_free(borderWidgetShader.shader);
	borderWidgetShader.shader = NULL;
	algl_shader_free(gridBorderWidgetShader.shader);
	gridBorderWidgetShader.shader = NULL;

	algl_shader_free(modelShader.shader);
	modelShader.shader = NULL;

	algl_shader_free(textShader.shader);
	textShader.shader = NULL;
	algl_texture_free(textShader.texture);
	textShader.texture = NULL;

	algl_shader_free(cursorShader.shader);
	cursorShader.shader = NULL;
	algl_texture_free(cursorShader.texture);
	cursorShader.texture = NULL;
}

static AlError init_shaders()
{
	BEGIN()

	plainWidgetShader.shader = NULL;
	borderWidgetShader.shader = NULL;
	gridBorderWidgetShader.shader = NULL;
	modelShader.shader = NULL;
	textShader.shader = NULL;
	textShader.texture = NULL;
	cursorShader.shader = NULL;
	cursorShader.texture = NULL;

	TRY(algl_shader_init_with_sources(&plainWidgetShader.shader,
		AL_VERT_SHADER(widget),
		AL_FRAG_SHADER(widget),
		NULL));
	ALGL_GET_UNIFORM(plainWidgetShader, viewportSize);
	ALGL_GET_UNIFORM(plainWidgetShader, min);
	ALGL_GET_UNIFORM(plainWidgetShader, size);
	ALGL_GET_UNIFORM(plainWidgetShader, fillColour);
	ALGL_GET_ATTRIB(plainWidgetShader, position);

	TRY(algl_shader_init_with_sources(&borderWidgetShader.shader,
		AL_VERT_SHADER(widget),
		AL_FRAG_SHADER(widget),
		"#define WITH_BORDER\n"));
	ALGL_GET_UNIFORM(borderWidgetShader, viewportSize);
	ALGL_GET_UNIFORM(borderWidgetShader, min);
	ALGL_GET_UNIFORM(borderWidgetShader, size);
	ALGL_GET_UNIFORM(borderWidgetShader, borderWidth);
	ALGL_GET_UNIFORM(borderWidgetShader, fillColour);
	ALGL_GET_UNIFORM(borderWidgetShader, borderColour);
	ALGL_GET_ATTRIB(borderWidgetShader, position);

	TRY(algl_shader_init_with_sources(&gridBorderWidgetShader.shader,
		AL_VERT_SHADER(widget),
		AL_FRAG_SHADER(widget),
		"#define WITH_BORDER\n"
		"#define WITH_GRID"));
	ALGL_GET_UNIFORM(gridBorderWidgetShader, viewportSize);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, min);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, size);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, borderWidth);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, gridSize);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, gridOffset);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, fillColour);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, borderColour);
	ALGL_GET_UNIFORM(gridBorderWidgetShader, gridColour);
	ALGL_GET_ATTRIB(gridBorderWidgetShader, position);

	TRY(algl_shader_init_with_sources(&modelShader.shader,
		AL_VERT_SHADER(model),
		AL_FRAG_SHADER(model),
		NULL));
	ALGL_GET_UNIFORM(modelShader, viewportSize);
	ALGL_GET_UNIFORM(modelShader, translate);
	ALGL_GET_UNIFORM(modelShader, scale);
	ALGL_GET_UNIFORM(modelShader, colour);
	ALGL_GET_ATTRIB(modelShader, position);
	ALGL_GET_ATTRIB(modelShader, param);

	TRY(algl_shader_init_with_sources(&textShader.shader,
		AL_VERT_SHADER(text),
		AL_FRAG_SHADER(text),
		NULL));
	ALGL_GET_UNIFORM(textShader, viewportSize);
	ALGL_GET_UNIFORM(textShader, min);
	ALGL_GET_UNIFORM(textShader, size);
	ALGL_GET_UNIFORM(textShader, charMin);
	ALGL_GET_UNIFORM(textShader, charSize);
	ALGL_GET_UNIFORM(textShader, font);
	ALGL_GET_UNIFORM(textShader, colour);
	ALGL_GET_UNIFORM(textShader, edge);
	ALGL_GET_ATTRIB(textShader, position);

	TRY(algl_texture_init(&textShader.texture));
	TRY(algl_texture_load_from_buffer(textShader.texture, images_font_png, images_font_png_size));

	fontInfo.numCharsW = 16;
	fontInfo.numCharsH = 16;
	fontInfo.charWidth = 0.5;
	fontInfo.xAdvance = 100.0 / 256.0;
	fontInfo.edgeCenter = 0.38;
	fontInfo.edgeSpread = 4.0;

	TRY(algl_shader_init_with_sources(&cursorShader.shader,
		AL_VERT_SHADER(cursor),
		AL_FRAG_SHADER(cursor),
		NULL));
	ALGL_GET_UNIFORM(cursorShader, viewportSize);
	ALGL_GET_UNIFORM(cursorShader, location);
	ALGL_GET_UNIFORM(cursorShader, size);
	ALGL_GET_UNIFORM(cursorShader, image);
	ALGL_GET_ATTRIB(cursorShader, position);

	TRY(algl_texture_init(&cursorShader.texture));
	TRY(algl_texture_load_from_buffer(cursorShader.texture, images_cursor_png, images_cursor_png_size));

	cursorInfo.textureSize = (Vec2){32, 32};
	cursorInfo.midPoint = (Vec2){16, 16};

	CATCH(
		free_shaders();
	)
	FINALLY()
}

static void update_viewport_size()
{
	viewportSize = algl_system_screen_size();

	glViewport(0, 0, viewportSize.x, viewportSize.y);

	glUseProgram(plainWidgetShader.shader->id);
	glUniform2f(plainWidgetShader.viewportSize, viewportSize.x, viewportSize.y);

	glUseProgram(borderWidgetShader.shader->id);
	glUniform2f(borderWidgetShader.viewportSize, viewportSize.x, viewportSize.y);

	glUseProgram(gridBorderWidgetShader.shader->id);
	glUniform2f(gridBorderWidgetShader.viewportSize, viewportSize.x, viewportSize.y);

	glUseProgram(modelShader.shader->id);
	glUniform2f(modelShader.viewportSize, viewportSize.x, viewportSize.y);

	glUseProgram(textShader.shader->id);
	glUniform2f(textShader.viewportSize, viewportSize.x, viewportSize.y);

	glUseProgram(cursorShader.shader->id);
	glUniform2f(cursorShader.viewportSize, viewportSize.x, viewportSize.y);
}

AlError graphics_system_init()
{
	BEGIN()

	TRY(algl_system_init());

	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	TRY(init_shaders());

	glGenBuffers(1, &plainVertices);
	glBindBuffer(GL_ARRAY_BUFFER, plainVertices);
	float vertices[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	update_viewport_size();

	CATCH(
		graphics_system_free();
	)
	FINALLY()
}

void graphics_system_free()
{
	glDeleteBuffers(1, &plainVertices);
	free_shaders();
	algl_system_free();
}

Vec2 graphics_screen_size()
{
	return viewportSize;
}

static void render_model(AlModel *model, Vec2 location, double scale)
{
	glUseProgram(modelShader.shader->id);
	algl_uniform_vec2(modelShader.translate, location);
	glUniform1f(modelShader.scale, scale);

	glEnableVertexAttribArray(modelShader.position);
	glEnableVertexAttribArray(modelShader.param);

	glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer);
	glVertexAttribPointer(modelShader.position, 2, GL_FLOAT, GL_FALSE, sizeof(AlGlModelVertex), (void *)offsetof(AlGlModelVertex, position));
	glVertexAttribPointer(modelShader.param, 3, GL_FLOAT, GL_FALSE, sizeof(AlGlModelVertex), (void *)offsetof(AlGlModelVertex, param));

	int start = 0;
	for (int i = 0; i < model->numPaths; i++) {
		algl_uniform_vec3(modelShader.colour, model->colours[i]);
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
	glUniform2f(textShader.size, charWidth, size);
	glUniform2f(textShader.charSize, 1.0 / fontInfo.numCharsW, 1.0 / fontInfo.numCharsH);
	algl_uniform_vec3(textShader.colour, colour);
	glUniform2f(textShader.edge, fontInfo.edgeCenter - edgeSpread, fontInfo.edgeCenter + edgeSpread);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textShader.texture->id);
	glUniform1i(textShader.font, 0);

	glBindBuffer(GL_ARRAY_BUFFER, plainVertices);

	glEnableVertexAttribArray(textShader.position);
	glVertexAttribPointer(textShader.position, 2, GL_FLOAT, GL_FALSE, 0, 0);

	struct TextReadState state;
	graphics_text_read_init(&state, text);
	uint8_t c;
	while ((c = graphics_text_read_next(&state))) {
		float x = (float)(c % fontInfo.numCharsW) / fontInfo.numCharsW;
		float y = (float)(c / fontInfo.numCharsW) / fontInfo.numCharsH;

		algl_uniform_vec2(textShader.min, location);
		glUniform2f(textShader.charMin, x, y);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		location.x += fontInfo.xAdvance * size;
	}
}

static void render_widget_main(AlWidget *widget, Box2 bounds)
{
	GLuint position;

	if (!widget->grid.size.x && !widget->grid.size.y) {
		if (!widget->border.width) {
			position = plainWidgetShader.position;
			glUseProgram(plainWidgetShader.shader->id);
			algl_uniform_vec2(plainWidgetShader.min, bounds.min);
			algl_uniform_vec2(plainWidgetShader.size, box2_size(bounds));
			algl_uniform_vec4(plainWidgetShader.fillColour, widget->fillColour);

		} else {
			position = borderWidgetShader.position;
			glUseProgram(borderWidgetShader.shader->id);
			algl_uniform_vec2(borderWidgetShader.min, bounds.min);
			algl_uniform_vec2(borderWidgetShader.size, box2_size(bounds));
			glUniform1f(borderWidgetShader.borderWidth, widget->border.width);
			algl_uniform_vec4(borderWidgetShader.fillColour, widget->fillColour);
			algl_uniform_vec4(borderWidgetShader.borderColour, widget->border.colour);
		}
	} else {
		position = gridBorderWidgetShader.position;
		glUseProgram(gridBorderWidgetShader.shader->id);
		algl_uniform_vec2(gridBorderWidgetShader.min, bounds.min);
		algl_uniform_vec2(gridBorderWidgetShader.size, box2_size(bounds));
		glUniform1f(gridBorderWidgetShader.borderWidth, widget->border.width);
		algl_uniform_vec2(gridBorderWidgetShader.gridSize, widget->grid.size);
		algl_uniform_vec2(gridBorderWidgetShader.gridOffset, widget->grid.offset);
		algl_uniform_vec4(gridBorderWidgetShader.fillColour, widget->fillColour);
		algl_uniform_vec4(gridBorderWidgetShader.borderColour, widget->border.colour);
		Vec3 gridColour = widget->grid.colour;
		algl_uniform_vec4(gridBorderWidgetShader.gridColour, (Vec4){
			gridColour.x, gridColour.y, gridColour.z, 1
		});
	}

	glBindBuffer(GL_ARRAY_BUFFER, plainVertices);
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void set_scissor(Box2 scissor)
{
	Vec2 size = box2_size(scissor);
	glScissor(scissor.min.x, scissor.min.y, size.x, size.y);
}

static void render_widget(AlWidget *widget, Vec2 translate, Box2 scissor)
{
	widget->valid = true;

	if (!widget->visible)
		return;

	Vec2 location = vec2_add(widget->location, translate);
	Box2 bounds = box2_add_vec2(widget->bounds, location);
	scissor = box2_round(box2_intersect(scissor, bounds));

	if (!box2_is_valid(scissor))
		return;

	if (!widget->passThrough) {
		set_scissor(scissor);

		render_widget_main(widget, bounds);

		if (widget->border.width > 0) {
			scissor = box2_expand(scissor, -widget->border.width);
			set_scissor(scissor);
		}

		if (widget->model.model) {
			render_model(widget->model.model, vec2_add(widget->model.location, location), widget->model.scale);
		}

		if (widget->text.value) {
			render_text(widget->text.value, widget->text.colour, vec2_add(widget->text.location, location), widget->text.size);
		}
	}

	FOR_EACH_WIDGET(child, widget) {
		render_widget(child, location, scissor);
	}
}

static void render_cursor(Vec2 location)
{
	location = vec2_subtract(location, cursorInfo.midPoint);

	glDisable(GL_SCISSOR_TEST);

	glUseProgram(cursorShader.shader->id);
	glUniform2f(cursorShader.location, location.x, location.y);
	glUniform2f(cursorShader.size, cursorInfo.textureSize.x, cursorInfo.textureSize.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cursorShader.texture->id);
	glUniform1i(cursorShader.image, 0);

	glBindBuffer(GL_ARRAY_BUFFER, plainVertices);

	glEnableVertexAttribArray(cursorShader.position);
	glVertexAttribPointer(cursorShader.position, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glEnable(GL_SCISSOR_TEST);
}

void graphics_render(AlWidget *root, bool renderCursor, Vec2 cursorLocation)
{
	if (!root->valid) {
		render_widget(root, (Vec2){0, 0}, (Box2){{0, 0}, viewportSize});

		if (renderCursor) {
			render_cursor(cursorLocation);
		}

		algl_system_swap_buffers();
	}
}
