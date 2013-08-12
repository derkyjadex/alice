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

static struct {
	AlHost *host;
	lua_State *lua;
	AlWrapper *wrapper;
} widgetSystem = {
	NULL, NULL, NULL
};

AlLuaKey widgetBindings;

static void _al_widget_init(AlWidget *widget)
{
	widget->next = NULL;
	widget->prev = NULL;
	widget->parent = NULL;
	widget->firstChild = NULL;
	widget->lastChild = NULL;

	widget->valid = false;
	widget->visible = true;
	widget->passThrough = false;
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
	TRY(al_wrapper_invoke_ctor(widgetSystem.wrapper, &widget));
	al_wrapper_retain(widgetSystem.wrapper, widget);

	*result = widget;

	PASS()
}

#define set_relation(widget, member, value) _set_relation(widget, &(widget)->member, value)

static void _set_relation(AlWidget *widget, AlWidget **member, AlWidget *value)
{
	if (*member) {
		al_widget_push_userdata(widget);
		al_widget_push_userdata(*member);
		al_wrapper_unreference(widgetSystem.wrapper);
	}

	if (value) {
		al_widget_push_userdata(widget);
		al_widget_push_userdata(value);
		al_wrapper_reference(widgetSystem.wrapper);
	}

	*member = value;
}

static void free_binding(AlWidget *widget, size_t bindingOffset)
{
	AlLuaKey *binding = (void *)widget + bindingOffset;
	if (*binding) {
		lua_pushlightuserdata(widgetSystem.lua, binding);
		lua_pushnil(widgetSystem.lua);
		lua_settable(widgetSystem.lua, LUA_REGISTRYINDEX);
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
	al_wrapper_release(widgetSystem.wrapper, widget);
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

static bool release_keyboard(AlWidget *widget, AlWidget *keyboardWidget)
{
	if (widget == keyboardWidget) {
		al_host_release_keyboard(widgetSystem.host);
		return true;
	}

	FOR_EACH_WIDGET(child, widget) {
		if (release_keyboard(child, keyboardWidget))
			return true;
	}

	return false;
}

void al_widget_remove(AlWidget *widget)
{
	release_keyboard(widget, al_host_get_keyboard_widget(widgetSystem.host));

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

	lua_State *L = widgetSystem.lua;

	if (*binding) {
		lua_pushlightuserdata(L, &widgetBindings);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushlightuserdata(L, binding);
		lua_gettable(L, -2);

		lua_insert(L, -nargs - 2);
		lua_pop(L, 1);

		al_widget_push_userdata(widget);
		lua_insert(L, -nargs - 1);

		TRY(al_script_call(L, nargs + 1));

	} else {
		lua_pop(L, nargs);
	}

	PASS()
}

AlError al_widget_send_down(AlWidget *widget, Vec2 location)
{
	lua_pushnumber(widgetSystem.lua, location.x);
	lua_pushnumber(widgetSystem.lua, location.y);

	return call_binding(widget, &widget->downBinding, 2);
}

AlError al_widget_send_up(AlWidget *widget, Vec2 location)
{
	lua_pushnumber(widgetSystem.lua, location.x);
	lua_pushnumber(widgetSystem.lua, location.y);

	return call_binding(widget, &widget->upBinding, 2);
}

AlError al_widget_send_motion(AlWidget *widget, Vec2 motion)
{
	lua_pushnumber(widgetSystem.lua, motion.x);
	lua_pushnumber(widgetSystem.lua, motion.y);

	return call_binding(widget, &widget->motionBinding, 2);
}

AlError al_widget_send_key(AlWidget *widget, SDL_Keycode key)
{
	lua_pushinteger(widgetSystem.lua, key);

	return call_binding(widget, &widget->keyBinding, 1);
}

AlError al_widget_send_text(AlWidget *widget, const char *text)
{
	lua_pushstring(widgetSystem.lua, text);

	return call_binding(widget, &widget->textBinding, 1);
}

AlError al_widget_send_keyboard_lost(AlWidget *widget)
{
	return call_binding(widget, &widget->keyboardLostBinding, 0);
}

AlWidget *al_widget_hit_test(AlWidget *widget, Vec2 location, Vec2 *hitLocation)
{
	if (!widget->visible)
		return NULL;

	location = vec2_subtract(location, widget->location);

	if (!box_contains(widget->bounds, location))
		return NULL;

	for (AlWidget *child = widget->lastChild; child; child = child->prev) {
		AlWidget *result = al_widget_hit_test(child, location, hitLocation);

		if (result) {
			return result;
		}
	}

	if (widget->passThrough)
		return NULL;

	if (hitLocation) {
		*hitLocation = location;
	}

	return widget;
}

void al_widget_push_userdata(AlWidget *widget)
{
	al_wrapper_push_userdata(widgetSystem.wrapper, widget);
}

void al_widget_reference()
{
	al_wrapper_reference(widgetSystem.wrapper);
}

static void wrapper_widget_free(lua_State *L, void *ptr)
{
	_al_widget_free(ptr);
}

AlError al_widget_systems_init(AlHost *host, lua_State *L, AlVars *vars)
{
	BEGIN()

	widgetSystem.lua = L;
	widgetSystem.host = host;

	TRY(al_wrapper_init(&widgetSystem.wrapper, "widget", sizeof(AlWidget), wrapper_widget_free));
	TRY(al_wrapper_wrap_ctor(widgetSystem.wrapper, al_widget_ctor, NULL));

	luaL_requiref(L, "widget", luaopen_widget, false);
	TRY(al_widget_system_register_vars(vars));

	lua_pushlightuserdata(L, &widgetBindings);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);

	CATCH(
		al_widget_systems_free();
	)
	FINALLY()
}

void al_widget_systems_free(void)
{
	al_wrapper_free(widgetSystem.wrapper);

	widgetSystem.host = NULL;
	widgetSystem.lua = NULL;
	widgetSystem.wrapper = NULL;
}
