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

Widget.prototype.next = make_wrapped_accessor(commands.widget_get_next, nil, Widget)
Widget.prototype.prev = make_wrapped_accessor(commands.widget_get_prev, nil, Widget)
Widget.prototype.parent = make_wrapped_accessor(commands.widget_get_parent, nil, Widget)
Widget.prototype.first_child = make_wrapped_accessor(commands.widget_get_first_child, nil, Widget)

local vars = {
	'location', 'bounds', 'fill_colour',
	'border_colour', 'border_width',
	'grid_size', 'grid_offset', 'grid_colour',
	'model_location', 'model_scale',
	'text', 'text_colour', 'text_size', 'text_location'}

for i,var in ipairs(vars) do
	Widget.prototype[var] = make_var_accessor('widget_' .. var)
end

function Widget.prototype.model(self, model)
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
Widget.prototype.remove = wrap_command(commands.widget_remove)

Widget.prototype.invalidate = wrap_command(commands.widget_invalidate)

Widget.prototype.bind_up = wrap_command(commands.widget_bind_up)
Widget.prototype.bind_down = wrap_command(commands.widget_bind_down)
function Widget.prototype.bind_motion(self, command)
	commands.widget_bind_motion(self._ptr, function(_, x, y)
		if self._grabbing then
			command(self, x, y)
		end
	end)
end
function Widget.prototype.bind_key(self, command)
	commands.widget_bind_key(self._ptr, function(_, key)
		command(self, key)
	end)
end
function Widget.prototype.bind_text(self, command)
	commands.widget_bind_text(self._ptr, function(_, text)
		command(self, text)
	end)
end

function Widget.prototype.grab_mouse(self)
	self._grabbing = true
	return commands.grab_mouse(self._ptr)
end
function Widget.prototype.release_mouse(self, x, y)
	self._grabbing = false
	commands.release_mouse(x, y)
end

function Widget.prototype.layout(self, left, width, right, bottom, height, top)
	local parent = self:parent()
	local parent_bounds = {parent:bounds()}
	parent_width = parent_bounds[3] - parent_bounds[1]
	parent_height = parent_bounds[4] - parent_bounds[2]

	if left == nil then
		left = parent_width - width - right
	elseif width == nil then
		width = parent_width - left - right
	end

	if bottom == nil then
		bottom = parent_height - height - top
	elseif height == nil then
		height = parent_height - bottom - top
	end

	self:location(left, bottom)
	self:bounds(0, 0, width, height)
	self:invalidate()
end

Widget.root = Widget(commands.get_root_widget())
