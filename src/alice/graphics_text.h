/*
 * Copyright (c) 2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef __GRAPHICS_TEXT_H__
#define __GRAPHICS_TEXT_H__


#include <stdint.h>

struct TextReadState {
	const char *input;
	size_t length;
};

void graphics_text_read_init(struct TextReadState *state, const char *input);
uint8_t graphics_text_read_next(struct TextReadState *state);

#endif
