/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "alice/widget.h"
#include "albase/lua.h"
#include "albase/script.h"
#include "widget_internal.h"
#include "widget_cmds.h"

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
		al_wrapper_push_userdata(L, *relation);
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

static void cmd_widget_set_model_null(lua_State *L, AlWidget *widget)
{
	if (widget->model.model) {
		al_model_unuse(widget->model.model);
		widget->model.model = NULL;
	}
}

static AlError cmd_widget_set_model_file(lua_State *L, AlWidget *widget)
{
	BEGIN()

	const char *filename = lua_tostring(L, 2);

	TRY(al_model_use_file(&widget->model.model, filename));

	if (widget->model.model) {
		al_model_unuse(widget->model.model);
	}

	CATCH(
		luaL_error(L, "Error loading model");
	)
	FINALLY()
}

static AlError cmd_widget_set_model_shape(lua_State *L, AlWidget *widget)
{
	BEGIN()

	AlModelShape *shape = lua_touserdata(L, 2);

	if (!widget->model.model) {
		TRY(al_model_use_shape(&widget->model.model, shape));
	} else {
		TRY(al_model_set_shape(widget->model.model, shape));
	}

	CATCH(
		luaL_error(L, "Error setting model shape");
	)
	FINALLY()
}

static int cmd_widget_set_model(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "set_model", 2);

	if (lua_isnil(L, 2)) {
		cmd_widget_set_model_null(L, widget);

	} else if (lua_isstring(L, 2)) {
		cmd_widget_set_model_file(L, widget);

	} else if (lua_isuserdata(L, 2)) {
		cmd_widget_set_model_shape(L, widget);

	} else {
		return luaL_error(L, "widget_set_model: invalid value for model");
	}

	lua_pushvalue(L, 1);

	return 1;
}

static int cmd_widget_add_child(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_child", 2);
	AlWidget *child = lua_touserdata(L, 2);

	al_widget_add_child(widget, child);

	return 1;
}

static int cmd_widget_add_sibling(lua_State *L)
{
	AlWidget *widget = cmd_accessor(L, "add_sibling", 2);
	AlWidget *sibling = lua_touserdata(L, 2);

	al_widget_add_sibling(widget, sibling);

	return 1;
}

static int cmd_widget_remove(lua_State *L)
{
	al_widget_remove(cmd_accessor(L, "remove", 1));

	return 1;
}

static int cmd_widget_invalidate(lua_State *L)
{
	al_widget_invalidate(cmd_accessor(L, "invalidate", 1));

	return 1;
}

static int cmd_widget_bind(lua_State *L, const char *name, size_t bindingOffset)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "widget_bind_%s: requires 2 arguments", name);

	AlWidget *widget = lua_touserdata(L, 1);
	AlLuaKey *binding = (void *)widget + bindingOffset;
	*binding = true;

	lua_pushlightuserdata(L, &widgetBindings);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, binding);
	lua_gettable(L, -2);

	if (!lua_isnil(L, -1)) {
		lua_pushvalue(L, 1);
		lua_pushvalue(L, -2);
		al_wrapper_reference(L);
	}

	lua_pop(L, 1);

	lua_pushlightuserdata(L, binding);
	lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pop(L, 1);

	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	al_wrapper_reference(L);

	lua_pop(L, 1);

	return 1;
}

#define BINDING(n, x) static int cmd_widget_bind_##n(lua_State *L) \
{ return cmd_widget_bind(L, #n, offsetof(AlWidget, x##Binding)); }

BINDING(up, up);
BINDING(down, down);
BINDING(motion, motion);
BINDING(key, key);
BINDING(text, text);
BINDING(keyboard_lost, keyboardLost);

#define REG_VAR(t, n, x) TRY(al_vars_register(L, (AlVarReg){ \
	.name = "widget."#n, \
	.type = t, \
	.scope = AL_VAR_INSTANCE, \
	.access = { \
		.instanceOffset = offsetof(AlWidget, x) \
	} \
}));

AlError al_widget_system_register_vars(lua_State *L)
{
	BEGIN()

	REG_VAR(AL_VAR_BOOL, visible, visible);
	REG_VAR(AL_VAR_BOOL, pass_through, passThrough);
	REG_VAR(AL_VAR_VEC2, location, location);
	REG_VAR(AL_VAR_BOX2, bounds, bounds);

	REG_VAR(AL_VAR_VEC4, fill_colour, fillColour);
	REG_VAR(AL_VAR_VEC4, border_colour, border.colour);
	REG_VAR(AL_VAR_INT, border_width, border.width);
	REG_VAR(AL_VAR_VEC2, grid_size, grid.size);
	REG_VAR(AL_VAR_VEC2, grid_offset, grid.offset);
	REG_VAR(AL_VAR_VEC3, grid_colour, grid.colour);
	REG_VAR(AL_VAR_VEC2, model_location, model.location);
	REG_VAR(AL_VAR_DOUBLE, model_scale, model.scale);
	REG_VAR(AL_VAR_STRING, text, text.value);
	REG_VAR(AL_VAR_VEC3, text_colour, text.colour);
	REG_VAR(AL_VAR_DOUBLE, text_size, text.size);
	REG_VAR(AL_VAR_VEC2, text_location, text.location);

	PASS()
}

static const luaL_Reg lib[] = {
	{"get_next", cmd_widget_get_next},
	{"get_prev", cmd_widget_get_prev},
	{"get_parent", cmd_widget_get_parent},
	{"get_first_child", cmd_widget_get_first_child},
	{"add_child", cmd_widget_add_child},
	{"add_sibling", cmd_widget_add_sibling},
	{"remove", cmd_widget_remove},
	{"invalidate", cmd_widget_invalidate},
	{"set_model", cmd_widget_set_model},
	{"bind_up", cmd_widget_bind_up},
	{"bind_down", cmd_widget_bind_down},
	{"bind_motion", cmd_widget_bind_motion},
	{"bind_key", cmd_widget_bind_key},
	{"bind_text", cmd_widget_bind_text},
	{"bind_keyboard_lost", cmd_widget_bind_keyboard_lost},
	{NULL, NULL}
};

int luaopen_widget(lua_State *L)
{
	luaL_newlib(L, lib);

	return 1;
}
