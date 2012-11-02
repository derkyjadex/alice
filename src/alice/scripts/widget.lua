-- widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

Widget = class(function(self, ptr)
	if ptr then
		commands.widget_register(self, ptr)
	else
		commands.widget_new(self)
	end

	self._grabbing = false
end)
commands.widget_register_ctor(Widget)

local vars = {
	'location', 'bounds', 'fill_colour',
	'border_colour', 'border_width',
	'grid_size', 'grid_offset', 'grid_colour',
	'model_location', 'model_scale',
	'text', 'text_colour', 'text_size', 'text_location'}

for i,var in ipairs(vars) do
	Widget.prototype[var] = make_var_accessor('widget_' .. var)
end

Widget.prototype.free = commands.widget_free
Widget.prototype.next = commands.widget_get_next
Widget.prototype.prev = commands.widget_get_prev
Widget.prototype.parent = commands.widget_get_parent
Widget.prototype.first_child = commands.widget_get_first_child
Widget.prototype.model = commands.widget_set_model
Widget.prototype.add_child = commands.widget_add_child
Widget.prototype.add_sibling = commands.widget_add_sibling
Widget.prototype.remove = commands.widget_remove
Widget.prototype.invalidate = commands.widget_invalidate
Widget.prototype.bind_up = commands.widget_bind_up
Widget.prototype.bind_down = commands.widget_bind_down
Widget.prototype.bind_key = commands.widget_bind_key
Widget.prototype.bind_text = commands.widget_bind_text
Widget.prototype.bind_keyboard_lost = commands.widget_bind_keyboard_lost
Widget.prototype.grab_keyboard = commands.grab_keyboard
Widget.prototype.release_keyboard = commands.release_keyboard

function Widget.prototype.bind_motion(self, command)
	commands.widget_bind_motion(self, function(_, x, y)
		if self._grabbing then
			command(self, x, y)
		end
	end)
end
function Widget.prototype.grab_mouse(self)
	self._grabbing = true
	return commands.grab_mouse(self)
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

Widget.root = commands.get_root_widget()
