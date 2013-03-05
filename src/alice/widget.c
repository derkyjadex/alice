/*
 * Copyright (c) 2011-2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <assert.h>

#include "alice/widget.h"
#include "albase/lua.h"
#include "albase/wrapper.h"
#include "albase/script.h"
#include "widget_internal.h"
#include "widget_cmds.h"

static lua_State *lua = NULL;
static AlWrapper *wrapper = NULL;
AlLuaKey widgetBindings;

static void _al_widget_init(AlWidget *widget)
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

	widget->downBinding = false;
	widget->upBinding = false;
	widget->motionBinding = false;
	widget->keyBinding = false;
	widget->textBinding = false;
	widget->keyboardLostBinding = false;
}

static int al_widget_ctor(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_call(L, 0, 1);
	AlWidget *widget = lua_touserdata(L, -1);

	_al_widget_init(widget);

	return 1;
}

AlError al_widget_init(AlWidget **result)
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
		al_widget_push_userdata(widget);
		al_widget_push_userdata(*member);
		al_wrapper_unreference(wrapper);
	}

	if (value) {
		al_widget_push_userdata(widget);
		al_widget_push_userdata(value);
		al_wrapper_reference(wrapper);
	}

	*member = value;
}

static void free_binding(AlWidget *widget, size_t bindingOffset)
{
	AlLuaKey *binding = (void *)widget + bindingOffset;
	if (*binding) {
		lua_pushlightuserdata(lua, binding);
		lua_pushnil(lua);
		lua_settable(lua, LUA_REGISTRYINDEX);
	}
}

static void _al_widget_free(AlWidget *widget)
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

void al_widget_free(AlWidget *widget)
{
	al_wrapper_release(wrapper, widget);
}

void al_widget_add_child(AlWidget *widget, AlWidget *child)
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
	al_widget_invalidate(child);
}

void al_widget_add_sibling(AlWidget *widget, AlWidget *sibling)
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
	al_widget_invalidate(sibling);
}

void al_widget_remove(AlWidget *widget)
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
		al_widget_invalidate(widget->parent);
	}

	set_relation(widget, prev, NULL);
	set_relation(widget, next, NULL);
	set_relation(widget, parent, NULL);
}

void al_widget_invalidate(AlWidget *widget)
{
	if (widget->valid) {
		widget->valid = false;

		if (widget->parent) {
			al_widget_invalidate(widget->parent);
		}
	}
}

static AlError call_binding(AlWidget *widget, AlLuaKey *binding, int nargs)
{
	BEGIN()

	if (*binding) {
		lua_pushlightuserdata(lua, &widgetBindings);
		lua_gettable(lua, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(lua, binding);
		lua_gettable(lua, -2);

		lua_insert(lua, -nargs - 2);
		lua_pop(lua, 1);

		al_widget_push_userdata(widget);
		lua_insert(lua, -nargs - 1);

		TRY(al_script_call(lua, nargs + 1));

	} else {
		lua_pop(lua, nargs);
	}

	PASS()
}

AlError al_widget_send_down(AlWidget *widget, Vec2 location)
{
	lua_pushnumber(lua, location.x);
	lua_pushnumber(lua, location.y);

	return call_binding(widget, &widget->downBinding, 2);
}

AlError al_widget_send_up(AlWidget *widget, Vec2 location)
{
	lua_pushnumber(lua, location.x);
	lua_pushnumber(lua, location.y);

	return call_binding(widget, &widget->upBinding, 2);
}

AlError al_widget_send_motion(AlWidget *widget, Vec2 motion)
{
	lua_pushnumber(lua, motion.x);
	lua_pushnumber(lua, motion.y);

	return call_binding(widget, &widget->motionBinding, 2);
}

AlError al_widget_send_key(AlWidget *widget, SDLKey key)
{
	lua_pushinteger(lua, key);

	return call_binding(widget, &widget->keyBinding, 1);
}

AlError al_widget_send_text(AlWidget *widget, const char *text)
{
	lua_pushstring(lua, text);

	return call_binding(widget, &widget->textBinding, 1);
}

AlError al_widget_send_keyboard_lost(AlWidget *widget)
{
	return call_binding(widget, &widget->keyboardLostBinding, 0);
}

AlWidget *al_widget_hit_test(AlWidget *widget, Vec2 location, Vec2 *hitLocation)
{
	AlWidget *result = NULL;

	if (box_contains(box_add_vec2(widget->bounds, widget->location), location)) {
		result = widget;

		for (AlWidget *child = widget->lastChild; child; child = child->prev) {
			AlWidget *childResult = al_widget_hit_test(
				child,
				vec2_subtract(location, widget->location),
				hitLocation);

			if (childResult) {
				result = childResult;
				break;
			}
		}

		if (result == widget && hitLocation) {
			*hitLocation = vec2_subtract(location, widget->location);
		}
	}

	return result;
}

void al_widget_push_userdata(AlWidget *widget)
{
	al_wrapper_push_userdata(wrapper, widget);
}

void al_widget_reference()
{
	al_wrapper_reference(wrapper);
}

static void wrapper_widget_free(lua_State *L, void *ptr)
{
	_al_widget_free(ptr);
}

static AlError al_widget_system_init_lua(lua_State *L)
{
	BEGIN()

	TRY(al_wrapper_init(&wrapper, L, sizeof(AlWidget), wrapper_widget_free));

	lua_pushlightuserdata(L, &widgetBindings);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	PASS()
}

AlError al_widget_systems_init(lua_State *L, AlCommands *commands, AlVars *vars)
{
	BEGIN()

	lua = L;

	TRY(al_widget_system_init_lua(L));
	TRY(al_widget_system_register_commands(commands));
	TRY(al_widget_system_register_vars(vars));

	TRY(al_wrapper_wrap_ctor(wrapper, al_widget_ctor, commands, NULL));
	TRY(al_wrapper_resgister_commands(wrapper, commands, "widget"));

	PASS()
}

void al_widget_systems_free(void)
{
	al_wrapper_free(wrapper);
	wrapper = NULL;
}
