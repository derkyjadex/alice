-- widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

Widget = class(function(self, ptr)
	if not ptr then
		ptr = commands.widget_new()
	end
	self._ptr = ptr
	self._grabbing = false
end)

Widget.prototype.free = make_free(commands.widget_free)

Widget.prototype.get_next = make_wrapped_accessor(commands.widget_get_next, Widget)
Widget.prototype.get_next = make_wrapped_accessor(commands.widget_get_next, Widget)
Widget.prototype.get_prev = make_wrapped_accessor(commands.widget_get_prev, Widget)
Widget.prototype.get_parent = make_wrapped_accessor(commands.widget_get_parent, Widget)
Widget.prototype.get_first_child = make_wrapped_accessor(commands.widget_get_first_child, Widget)

local vars = {
	'location', 'bounds', 'fill_colour',
	'border_colour', 'border_width',
	'grid_size', 'grid_offset', 'grid_colour',
	'model_location', 'model_scale',
	'text', 'text_colour', 'text_size', 'text_location'}

for i,var in ipairs(vars) do
	Widget.prototype['get_' .. var] = make_var_getter('widget_' .. var)
	Widget.prototype['set_' .. var] = make_var_setter('widget_' .. var)
end

Widget.prototype.set_model = make_accessor(commands.widget_set_model)

function Widget.prototype.set_model(self, model)
	if type(model) == 'table' then
		model = model._ptr
	end
	commands.widget_set_model(self._ptr, model)
end

function Widget.prototype.add_child(self, child)
	commands.widget_add_child(self._ptr, child._ptr)
end
function Widget.prototype.add_sibling(self, sibling)
	commands.widget_add_sibling(self._ptr, child._ptr)
end
Widget.prototype.remove = make_accessor(commands.widget_remove)

Widget.prototype.bind_up = make_accessor(commands.widget_bind_up)
Widget.prototype.bind_down = make_accessor(commands.widget_bind_down)
function Widget.prototype.bind_motion(self, command)
	commands.widget_bind_motion(self._ptr, function(_, x, y)
		if self._grabbing then
			command(self, x, y)
		end
	end)
end

Widget.prototype.grab_mouse = function(self)
	self._grabbing = true
	return commands.grab_mouse(self._ptr)
end
Widget.prototype.release_mouse = function(self, x, y)
	self._grabbing = false
	commands.release_mouse(x, y)
end

Widget.root = Widget(commands.get_root_widget())
