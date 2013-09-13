/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>

#include "albase/data.h"

static void print_indent(int n)
{
	for (int i = 0; i < n; i++) {
		printf("  ");
	}
}

static void print_tag(AlDataTag tag)
{
	char a = (0x000000FF & tag) >> 0;
	char b = (0x0000FF00 & tag) >> 8;
	char c = (0x00FF0000 & tag) >> 16;
	char d = (0xFF000000 & tag) >> 24;

	printf("%c%c%c%c", a, b, c, d);
}

static void print_value(AlVarType type, void *value)
{
	switch (type) {
		case AL_VAR_BOOL:
			if (*(bool *)value) {
				printf("true");
			} else {
				printf("false");
			}
			break;

		case AL_VAR_INT:
			printf("%d", *(int32_t *)value);
			break;

		case AL_VAR_DOUBLE:
			printf("%f", *(double *)value);
			break;

		case AL_VAR_VEC2:
			printf("[%f, %f]",
				   ((Vec2 *)value)->x,
				   ((Vec2 *)value)->y);
			break;

		case AL_VAR_VEC3:
			printf("[%f, %f, %f]",
				   ((Vec3 *)value)->x,
				   ((Vec3 *)value)->y,
				   ((Vec3 *)value)->z);
			break;

		case AL_VAR_VEC4:
			printf("[%f, %f, %f, %f]",
				   ((Vec4 *)value)->x,
				   ((Vec4 *)value)->y,
				   ((Vec4 *)value)->z,
				   ((Vec4 *)value)->w);
			break;

		case AL_VAR_BOX2:
			printf("[%f, %f]-[%f, %f]",
				   ((Box2 *)value)->min.x,
				   ((Box2 *)value)->min.y,
				   ((Box2 *)value)->max.x,
				   ((Box2 *)value)->max.y);
			break;

		case AL_VAR_STRING:
			printf("\"%s\"", *(char **)value);
			break;

		case AL_VAR_BLOB:
			printf("0x");
			for (int i = 0; i < ((AlBlob *)value)->length; i++) {
				printf("%02x", ((AlBlob *)value)->bytes[i]);
			}
			break;
	}
}

static size_t get_var_size(AlVarType type)
{
	switch (type) {
		case AL_VAR_BOOL: return sizeof(bool);
		case AL_VAR_INT: return sizeof(int32_t);
		case AL_VAR_DOUBLE: return sizeof(double);
		case AL_VAR_VEC2: return sizeof(Vec2);
		case AL_VAR_VEC3: return sizeof(Vec3);
		case AL_VAR_VEC4: return sizeof(Vec4);
		case AL_VAR_BOX2: return sizeof(Box2);
		default: return 0;
	}
}

static void print_array(AlDataItem item, int indent)
{
	printf("{\n");
	void *items = item.value.array.items;
	size_t itemSize = get_var_size(item.type);
	for (uint64_t i = 0; i < item.value.array.length; i++) {
		print_indent(indent + 1);
		print_value(item.type, items);
		printf("\n");
		items += itemSize;
	}
	print_indent(indent);
	printf("}");

}

static AlError print_data(AlData *data, int indent)
{
	BEGIN()

	AlDataItem item;
	bool first = true;

	while (true) {
		TRY(al_data_read(data, &item));

		switch (item.type) {
			case AL_TOKEN_START:
				if ((indent == 0 && !first) || indent > 0) {
					printf("\n");
				}
				print_indent(indent);

				first = false;

				printf("(");
				print_data(data, indent + 1);
				break;

			case AL_TOKEN_END:
				printf(")");
				RETURN();

			case AL_TOKEN_EOF:
				RETURN();

			case AL_TOKEN_TAG:
				if (first) {
					first = false;
				} else {
					printf(" ");
				}

				print_tag(item.value.tag);
				break;

			case AL_VAR_BOOL:
			case AL_VAR_INT:
			case AL_VAR_DOUBLE:
			case AL_VAR_VEC2:
			case AL_VAR_VEC3:
			case AL_VAR_VEC4:
			case AL_VAR_BOX2:
			case AL_VAR_STRING:
			case AL_VAR_BLOB:
				if (first) {
					first = false;
				} else {
					printf(" ");
				}

				if (!item.array) {
					print_value(item.type, &item.value);

				} else {
					print_array(item, indent);
				}
				break;
		}
	}

	PASS()
}

int main(int argc, char *argv[])
{
	BEGIN()

	AlStream *file = NULL;
	AlData *data = NULL;
	TRY(al_stream_init_file(&file, stdin, false, "<stdin>"));
	TRY(al_data_init(&data, file));
	TRY(print_data(data, 0));
	printf("\n");

	CATCH()
	FINALLY({
		al_data_free(data);
		al_stream_free(file);
	})
}
