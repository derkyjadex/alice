/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "alice/widget.h"
#include "albase/lua.h"

AlError widget_init(AlWidget **result, lua_State *lua, AlCommands *commands)
{
	BEGIN()

	AlWidget *widget = NULL;
	TRY(al_malloc(&widget, sizeof(AlWidget), 1));

	widget->next = NULL;
	widget->prev = NULL;
	widget->parent = NULL;
	widget->firstChild = NULL;
	widget->lastChild = NULL;

	widget->location = (Vec2){0, 0};
	widget->bounds = (Box){{0, 0}, {0, 0}};
	widget->fillColour = (Vec4){1, 1, 1, 1};
	widget->border.colour = (Vec4){1, 1, 1, 1};
	widget->border.width = 0;
	widget->model.model = NULL;
	widget->model.location = (Vec2){0, 0};
	widget->model.scale = 1;
	widget->grid.size = (Vec2){0, 0};
	widget->grid.offset = (Vec2){0, 0};
	widget->grid.colour = (Vec3){1, 1, 1};
	widget->text.value = NULL;
	widget->text.colour = (Vec3){1, 1, 1};
	widget->text.size = 12;
	widget->text.location = (Vec2){0, 0};

	widget->lua = lua;
	widget->commands = commands;
	widget->downBinding = false;
	widget->upBinding = false;
	widget->motionBinding = false;

	*result = widget;

	CATCH(
		widget_free(widget);
	)
	FINALLY()
}

void widget_free(AlWidget *widget)
{
	if (widget) {
		AlWidget *child = widget->firstChild;
		while (child) {
			AlWidget *next = child->next;
			widget_free(child);
			child = next;
		}

		al_model_unuse(widget->model.model);
		free(widget->text.value);

		free(widget);
	}
}

void widget_add_child(AlWidget *widget, AlWidget *child)
{
	assert(!child->next && !child->prev && !child->parent);

	if (widget->lastChild) {
		widget->lastChild->next = child;
		child->prev = widget->lastChild;

	} else {
		widget->firstChild = child;
	}

	widget->lastChild = child;
	child->parent = widget;
}

void widget_add_sibling(AlWidget *widget, AlWidget *sibling)
{
	assert(!sibling->next && !sibling->prev && !sibling->parent);

	if (widget->next) {
		widget->next->prev = sibling;
		sibling->next = widget->next;

	} else if (widget->parent) {
		widget->parent->lastChild = sibling;
	}

	widget->next = sibling;
	sibling->prev = widget;
	sibling->parent = widget->parent;
}

void widget_remove(AlWidget *widget)
{
	if (widget->next) {
		widget->next->prev = widget->prev;

	} else if (widget->parent) {
		widget->parent->lastChild = widget->prev;
	}

	if (widget->prev) {
		widget->prev->next = widget->next;

	} else if (widget->parent) {
		widget->parent->firstChild = widget->next;
	}

	widget->prev = NULL;
	widget->next = NULL;
	widget->parent = NULL;
}

static AlError widget_send(AlWidget *widget, AlLuaKey *binding)
{
	BEGIN()

	if (*binding) {
		lua_State *L = widget->lua;

		lua_pushlightuserdata(L, binding);
		lua_gettable(L, LUA_REGISTRYINDEX);
		TRY(al_commands_enqueue(widget->commands));
	}

	PASS()
}

AlError widget_send_down(AlWidget *widget)
{
	return widget_send(widget, &widget->downBinding);
}

AlError widget_send_up(AlWidget *widget)
{
	return widget_send(widget, &widget->upBinding);
}

AlError widget_send_motion(AlWidget *widget, Vec2 motion)
{
	BEGIN()

	if (widget->motionBinding) {
		lua_State *L = widget->lua;

		lua_newtable(L);
		lua_pushinteger(L, 1);
		lua_pushlightuserdata(L, &widget->motionBinding);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_settable(L, -3);

		lua_pushinteger(L, 2);
		lua_pushlightuserdata(L, widget);
		lua_settable(L, -3);

		lua_pushinteger(L, 3);
		lua_pushnumber(L, motion.x);
		lua_settable(L, -3);

		lua_pushinteger(L, 4);
		lua_pushnumber(L, motion.y);
		lua_settable(L, -3);

		TRY(al_commands_enqueue(widget->commands));
	}

	PASS()
}

AlWidget *widget_hit_test(AlWidget *widget, Vec2 location)
{
	AlWidget *result = NULL;

	if (box_contains(box_add_vec2(widget->bounds, widget->location), location)) {
		result = widget;

		for (AlWidget *child = widget->lastChild; child; child = child->prev) {
			AlWidget *childResult =  widget_hit_test(child, vec2_subtract(location, widget->location));
			if (childResult) {
				result = childResult;
				break;
			}
		}
	}

	return result;
}

static int cmd_widget_new(lua_State *L)
{
	BEGIN()

	AlCommands *commands = lua_touserdata(L, lua_upvalueindex(1));

	AlWidget *widget = NULL;
	TRY(widget_init(&widget, L, commands));

	lua_pushlightuserdata(L, widget);

	CATCH(
		return luaL_error(L, "Error creating widget");
	)
	FINALLY(
		return 1;
	)
}

static AlWidget *cmd_accessor(lua_State *L, const char *name, int numArgs)
{
	if (lua_gettop(L) != numArgs) {
		luaL_error(L, "widget_%s: requires %d argument(s)", name, numArgs);
	}

	AlWidget *widget = lua_touserdata(L, 1);
	if (!widget) {
		luaL_error(L, "widget_%s: first argument must be a Widget", name);
	}

	return widget;
}

static int cmd_widget_free(lua_State *L)
{
	widget_free(cmd_accessor(L, "free", 1));

	return 0;
}

static int cmd_widget_get_relation(lua_State *L, const char *name, size_t offset)
{
	AlWidget *widget = cmd_accessor(L, name, 1);
	AlWidget **relation = (void *)widget + offset;

	if (*relation) {
		lua_pushlightuserdata(L, *relation);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

#define RELATION_GETTER(n, x) static int cmd_widget_get_##n(lua_State *L) \
{ return cmd_widget_get_relation(L, "get_"#n, offsetof(AlWidget, x)); }

RELATION_GETTER(next, next);
RELATION_GETTER(prev, prev);
RELATION_GETTER(parent, parent);
RELATION_GETTER(first_child, firstChild);

static int cmd_widget_set_model_null(lua_State *L, AlWidget *widget)
{
	if (widget->model.model) {
		al_model_unuse(widget->model.model);
		widget->model.model = NULL;
	}

	return 0;
}

static int cmd_widget_set_model_file(lua_State *L, AlWidget *widget)
{
	BEGIN()

	const char *filename = lua_tostring(L, 2);

	TRY(al_model_use_file(&widget->model.model, filename));

	if (widget->model.model) {
		al_model_unuse(widget->model.model);
	}

	CATCH(
		return luaL_error(L, "Error loading model");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_widget_set_model_shape(lua_State *L, AlWidget *widget)
{
	BEGIN()

	AlModelShape *shape = lua_touserdata(L, 2);

	if (!widget->model.model) {
		TRY(al_model_use_shape(&widget->model.model, shape));
	} else {
		TRY(al_model_set_shape(widget->model.model, shape));
	}

	CATCH(
		return luaL_error(L, "Error setting model shape");
	)
	FINALLY(
		return 0;
	)
}

static int cmd_widget_set_model(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "set_model", 2);

	if (lua_isnil(L, 2)) {
		return cmd_widget_set_model_null(L, widget);

	} else if (lua_isstring(L, 2)) {
		return cmd_widget_set_model_file(L, widget);

	} else if (lua_islightuserdata(L, 2)) {
		return cmd_widget_set_model_shape(L, widget);

	} else {
		return luaL_error(L, "widget_set_model: invalid value for model");
	}
}

static int cmd_widget_add_child(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_child", 2);
	AlWidget *child = lua_touserdata(L, -1);

	widget_add_child(widget, child);

	return 0;
}

static int cmd_widget_add_sibling(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_sibling", 2);
	AlWidget *sibling = lua_touserdata(L, -1);

	widget_add_sibling(widget, sibling);

	return 0;
}

static int cmd_widget_remove(lua_State *L)
{
	widget_remove(cmd_accessor(L, "remove", 1));

	return 0;
}

static int cmd_widget_bind(lua_State *L, const char *name, size_t bindingOffset)
{
	int n = lua_gettop(L);
	if (n < 2) {
		return luaL_error(L, "%s: requires at least two arguments", name);
	}

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	AlWidget *widget = lua_touserdata(L, 1);
	AlLuaKey *binding = (void *)widget + bindingOffset;
	*binding = true;

	lua_newtable(L);

	int i;
	for (i = 1; i <= n - 1; i++) {
		lua_pushinteger(L, i);
		lua_pushvalue(L, i - n - 2);
		lua_settable(L, -3);
	}

	lua_pushlightuserdata(L, binding);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, n + 1);

	return 0;
}

static int cmd_widget_bind_up(lua_State *L)
{
	return cmd_widget_bind(L, "bind_up", offsetof(AlWidget, upBinding));
}

static int cmd_widget_bind_down(lua_State *L)
{
	return cmd_widget_bind(L, "bind_down", offsetof(AlWidget, downBinding));
}

static int cmd_widget_bind_motion(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "widget_bind_motion: requires 2 arguments");

	AlWidget *widget = lua_touserdata(L, -2);
	widget->motionBinding = true;

	lua_pushlightuserdata(L, &widget->motionBinding);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 2);

	return 0;
}

#define REG_CMD(x) TRY(al_commands_register(commands, "widget_"#x, cmd_widget_ ## x, NULL))

AlError widget_register_commands(AlCommands *commands)
{
	BEGIN()

	TRY(al_commands_register(commands, "widget_new", cmd_widget_new, commands));
	REG_CMD(free);

	REG_CMD(get_next);
	REG_CMD(get_prev);
	REG_CMD(get_parent);
	REG_CMD(get_first_child);

	REG_CMD(add_child);
	REG_CMD(add_sibling);
	REG_CMD(remove);

	REG_CMD(set_model);

	REG_CMD(bind_up);
	REG_CMD(bind_down);
	REG_CMD(bind_motion);

	PASS()
}

#define REG_VAR(t, n, x) TRY(al_vars_register_instance(vars, "widget_"#n, t, offsetof(AlWidget, x)))

AlError widget_register_vars(AlVars *vars)
{
	BEGIN()

	REG_VAR(VAR_VEC2, location, location);
	REG_VAR(VAR_BOX, bounds, bounds);

	REG_VAR(VAR_VEC4, fill_colour, fillColour);
	REG_VAR(VAR_VEC4, border_colour, border.colour);
	REG_VAR(VAR_INT, border_width, border.width);
	REG_VAR(VAR_VEC2, grid_size, grid.size);
	REG_VAR(VAR_VEC2, grid_offset, grid.offset);
	REG_VAR(VAR_VEC3, grid_colour, grid.colour);
	REG_VAR(VAR_VEC2, model_location, model.location);
	REG_VAR(VAR_DOUBLE, model_scale, model.scale);
	REG_VAR(VAR_STRING, text, text.value);
	REG_VAR(VAR_VEC3, text_colour, text.colour);
	REG_VAR(VAR_DOUBLE, text_size, text.size);
	REG_VAR(VAR_VEC2, text_location, text.location);

	PASS()
}
