/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "alice/widget.h"
#include "albase/lua.h"
#include "albase/wrapper.h"

static AlWrapper *wrapper = NULL;
static AlLuaKey bindings;

static void _widget_init(AlWidget *widget, lua_State *lua, AlCommands *commands)
{
	widget->next = NULL;
	widget->prev = NULL;
	widget->parent = NULL;
	widget->firstChild = NULL;
	widget->lastChild = NULL;

	widget->valid = false;
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
	widget->keyBinding = false;
	widget->textBinding = false;
	widget->keyboardLostBinding = false;
}

AlError widget_init(AlWidget **result)
{
	BEGIN()

	AlWidget *widget = NULL;
	TRY(al_wrapper_invoke_ctor(wrapper, &widget));
	al_wrapper_retain(wrapper, widget);

	*result = widget;

	PASS()
}

#define set_relation(widget, member, value) _set_relation(widget, &(widget)->member, value)

static void _set_relation(AlWidget *widget, AlWidget **member, AlWidget *value)
{
	if (*member) {
		widget_push_userdata(widget);
		widget_push_userdata(*member);
		al_wrapper_unreference(wrapper);
	}

	if (value) {
		widget_push_userdata(widget);
		widget_push_userdata(value);
		al_wrapper_reference(wrapper);
	}

	*member = value;
}

static void free_binding(AlWidget *widget, size_t bindingOffset)
{
	AlLuaKey *binding = (void *)widget + bindingOffset;
	if (*binding) {
		lua_State *L = widget->lua;

		lua_pushlightuserdata(L, binding);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
}

static void _widget_free(AlWidget *widget)
{
	if (widget) {
		al_model_unuse(widget->model.model);
		free(widget->text.value);

		free_binding(widget, offsetof(AlWidget, downBinding));
		free_binding(widget, offsetof(AlWidget, upBinding));
		free_binding(widget, offsetof(AlWidget, motionBinding));
		free_binding(widget, offsetof(AlWidget, keyBinding));
		free_binding(widget, offsetof(AlWidget, textBinding));
		free_binding(widget, offsetof(AlWidget, keyboardLostBinding));
	}
}

void widget_free(AlWidget *widget)
{
	al_wrapper_release(wrapper, widget);
}

void widget_add_child(AlWidget *widget, AlWidget *child)
{
	assert(!child->next && !child->prev && !child->parent);

	if (widget->lastChild) {
		set_relation(widget->lastChild, next, child);
		set_relation(child, prev, widget->lastChild);

	} else {
		set_relation(widget, firstChild, child);
	}

	set_relation(widget, lastChild, child);
	set_relation(child, parent, widget);

	child->valid = true;
	widget_invalidate(child);
}

void widget_add_sibling(AlWidget *widget, AlWidget *sibling)
{
	assert(!sibling->next && !sibling->prev && !sibling->parent);

	if (widget->next) {
		set_relation(widget->next, prev, sibling);
		set_relation(sibling, next, widget->next);

	} else if (widget->parent) {
		set_relation(widget->parent, lastChild, sibling);
	}

	set_relation(widget, next, sibling);
	set_relation(sibling, prev, widget);
	set_relation(sibling, parent, widget->parent);

	sibling->valid = true;
	widget_invalidate(sibling);
}

void widget_remove(AlWidget *widget)
{
	if (widget->next) {
		set_relation(widget->next, prev, widget->prev);

	} else if (widget->parent) {
		set_relation(widget->parent, lastChild, widget->prev);
	}

	if (widget->prev) {
		set_relation(widget->prev, next, widget->next);

	} else if (widget->parent) {
		set_relation(widget->parent, firstChild, widget->next);
	}

	if (widget->parent) {
		widget_invalidate(widget->parent);
	}

	set_relation(widget, prev, NULL);
	set_relation(widget, next, NULL);
	set_relation(widget, parent, NULL);
}

void widget_invalidate(AlWidget *widget)
{
	if (widget->valid) {
		widget->valid = false;

		if (widget->parent) {
			widget_invalidate(widget->parent);
		}
	}
}

static void push_bound_value(AlWidget *widget, AlLuaKey *binding)
{
	lua_State *L = widget->lua;

	lua_pushlightuserdata(L, &bindings);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, binding);
	lua_gettable(L, -2);

	lua_replace(L, -2);
}

static AlError widget_send_plain(AlWidget *widget, AlLuaKey *binding)
{
	BEGIN()

	if (*binding) {
		push_bound_value(widget, binding);
		TRY(al_commands_enqueue(widget->commands));
	}

	PASS()
}

AlError widget_send_down(AlWidget *widget)
{
	return widget_send_plain(widget, &widget->downBinding);
}

AlError widget_send_up(AlWidget *widget)
{
	return widget_send_plain(widget, &widget->upBinding);
}

AlError widget_send_motion(AlWidget *widget, Vec2 motion)
{
	BEGIN()

	if (widget->motionBinding) {
		lua_State *L = widget->lua;

		lua_newtable(L);
		lua_pushinteger(L, 1);
		push_bound_value(widget, &widget->motionBinding);
		lua_settable(L, -3);

		lua_pushinteger(L, 2);
		widget_push_userdata(widget);
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

AlError widget_send_key(AlWidget *widget, SDLKey key)
{
	BEGIN()

	if (widget->keyBinding) {
		lua_State *L = widget->lua;

		lua_newtable(L);
		lua_pushinteger(L, 1);
		push_bound_value(widget, &widget->keyBinding);
		lua_settable(L, -3);

		lua_pushinteger(L, 2);
		widget_push_userdata(widget);
		lua_settable(L, -3);

		lua_pushinteger(L, 3);
		lua_pushinteger(L, key);
		lua_settable(L, -3);

		TRY(al_commands_enqueue(widget->commands));
	}

	PASS()
}

AlError widget_send_text(AlWidget *widget, const char *text)
{
	BEGIN()

	if (widget->textBinding) {
		lua_State *L = widget->lua;

		lua_newtable(L);
		lua_pushinteger(L, 1);
		push_bound_value(widget, &widget->textBinding);
		lua_settable(L, -3);

		lua_pushinteger(L, 2);
		widget_push_userdata(widget);
		lua_settable(L, -3);

		lua_pushinteger(L, 3);
		lua_pushstring(L, text);
		lua_settable(L, -3);

		TRY(al_commands_enqueue(widget->commands));
	}

	PASS()
}

AlError widget_send_keyboard_lost(AlWidget *widget)
{
	return widget_send_plain(widget, &widget->keyboardLostBinding);
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

void widget_push_userdata(AlWidget *widget)
{
	al_wrapper_push_userdata(wrapper, widget);
}

static int cmd_widget_new(lua_State *L)
{
	BEGIN()

	AlCommands *commands = lua_touserdata(L, lua_upvalueindex(1));

	AlWidget *widget = NULL;
	TRY(al_wrapper_create(wrapper, &widget));
	_widget_init(widget, L, commands);

	widget_push_userdata(widget);

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

	return lua_touserdata(L, 1);
}

static int cmd_widget_get_relation(lua_State *L, const char *name, size_t offset)
{
	AlWidget *widget = cmd_accessor(L, name, 1);
	AlWidget **relation = (void *)widget + offset;

	if (*relation) {
		widget_push_userdata(*relation);
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

	} else if (lua_istable(L, 2)) {
		return cmd_widget_set_model_shape(L, widget);

	} else {
		return luaL_error(L, "widget_set_model: invalid value for model");
	}
}

static int cmd_widget_add_child(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_child", 2);
	AlWidget *child = lua_touserdata(L, 2);

	widget_add_child(widget, child);

	return 0;
}

static int cmd_widget_add_sibling(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_sibling", 2);
	AlWidget *sibling = lua_touserdata(L, 2);

	widget_add_sibling(widget, sibling);

	return 0;
}

static int cmd_widget_remove(lua_State *L)
{
	widget_remove(cmd_accessor(L, "remove", 1));

	return 0;
}

static int cmd_widget_invalidate(lua_State *L)
{
	widget_invalidate(cmd_accessor(L, "invalidate", 1));

	return 0;
}

static int cmd_widget_bind(lua_State *L, size_t bindingOffset)
{
	AlWidget *widget = lua_touserdata(L, 1);
	AlLuaKey *binding = (void *)widget + bindingOffset;
	*binding = true;

	lua_pushlightuserdata(L, &bindings);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, binding);
	lua_gettable(L, -2);

	if (!lua_isnil(L, -1)) {
		lua_pushvalue(L, 1);
		lua_pushvalue(L, -2);
		al_wrapper_unreference(wrapper);
	}

	lua_pop(L, 1);

	lua_pushlightuserdata(L, binding);
	lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);

	al_wrapper_reference(wrapper);

	return 0;
}

static int cmd_widget_bind_plain(lua_State *L, const char *name, size_t bindingOffset)
{
	int n = lua_gettop(L);
	if (n < 2)
		return luaL_error(L, "widget_bind_%s: requires at least two arguments", name);

	lua_newtable(L);

	for (int i = 1; i <= n - 1; i++) {
		lua_pushinteger(L, i);
		lua_pushvalue(L, i + 1);
		lua_settable(L, -3);
	}

	lua_replace(L, 2);
	lua_settop(L, 2);

	return cmd_widget_bind(L, bindingOffset);
}

static int cmd_widget_bind_data(lua_State *L, const char *name, size_t bindingOffset)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "widget_bind_%s: requires 2 arguments", name);

	return cmd_widget_bind(L, bindingOffset);
}

static int cmd_widget_bind_up(lua_State *L)
{
	return cmd_widget_bind_plain(L, "up", offsetof(AlWidget, upBinding));
}

static int cmd_widget_bind_down(lua_State *L)
{
	return cmd_widget_bind_plain(L, "down", offsetof(AlWidget, downBinding));
}

static int cmd_widget_bind_motion(lua_State *L)
{
	return cmd_widget_bind_data(L, "motion", offsetof(AlWidget, motionBinding));
}

static int cmd_widget_bind_key(lua_State *L)
{
	return cmd_widget_bind_data(L, "key", offsetof(AlWidget, keyBinding));
}

static int cmd_widget_bind_text(lua_State *L)
{
	return cmd_widget_bind_data(L, "text", offsetof(AlWidget, textBinding));
}

static int cmd_widget_bind_keyboard_lost(lua_State *L)
{
	return cmd_widget_bind_plain(L, "keyboard_lost", offsetof(AlWidget, keyboardLostBinding));
}

static void wrapper_widget_free(lua_State *L, void *ptr)
{
	_widget_free(ptr);
}

static AlError widget_system_init_lua(lua_State *L)
{
	BEGIN()

	TRY(al_wrapper_init(&wrapper, L, sizeof(AlWidget), wrapper_widget_free));

	lua_pushlightuserdata(L, &bindings);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	PASS()
}

#define REG_CMD(x) TRY(al_commands_register(commands, "widget_"#x, cmd_widget_ ## x, NULL))

static AlError widget_system_register_commands(AlCommands *commands)
{
	BEGIN()

	TRY(al_wrapper_register_register_ctor_command(wrapper, "widget_register_ctor", commands));
	TRY(al_commands_register(commands, "widget_new", cmd_widget_new, commands, NULL));

	REG_CMD(get_next);
	REG_CMD(get_prev);
	REG_CMD(get_parent);
	REG_CMD(get_first_child);

	REG_CMD(add_child);
	REG_CMD(add_sibling);
	REG_CMD(remove);

	REG_CMD(invalidate);

	REG_CMD(set_model);

	REG_CMD(bind_up);
	REG_CMD(bind_down);
	REG_CMD(bind_motion);
	REG_CMD(bind_key);
	REG_CMD(bind_text);
	REG_CMD(bind_keyboard_lost);

	PASS()
}

#define REG_VAR(t, n, x) TRY(al_vars_register_instance(vars, "widget_"#n, t, offsetof(AlWidget, x)))

static AlError widget_system_register_vars(AlVars *vars)
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

AlError widget_system_init(lua_State *L, AlCommands *commands, AlVars *vars)
{
	BEGIN()

	TRY(widget_system_init_lua(L));
	TRY(widget_system_register_commands(commands));
	TRY(widget_system_register_vars(vars));

	lua_pushlightuserdata(L, commands);
	lua_pushcclosure(L, cmd_widget_new, 1);
	TRY(al_wrapper_register_ctor(wrapper));

	PASS()
}