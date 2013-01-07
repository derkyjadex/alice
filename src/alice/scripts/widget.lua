-- widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

Widget = wrap('widget', function(self)
	self._grabbing = false
end)

local vars = {
	'location', 'bounds', 'fill_colour',
	'border_colour', 'border_width',
	'grid_size', 'grid_offset', 'grid_colour',
	'model_location', 'model_scale',
	'text', 'text_colour', 'text_size', 'text_location'}

for i,var in ipairs(vars) do
	Widget.prototype[var] = make_var_accessor('widget_' .. var)
end

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

function Widget.prototype:bind_motion(command)
	return commands.widget_bind_motion(self, function(_, x, y)
		if self._grabbing then
			command(self, x, y)
		end
	end)
end
function Widget.prototype:grab_mouse()
	self._grabbing = true
	return commands.grab_mouse(self)
end
function Widget.prototype:release_mouse(x, y)
	self._grabbing = false
	commands.release_mouse(x, y)
end

function Widget.prototype:layout(left, width, right, bottom, height, top, offset_x, offset_y)
	local parent = self:parent()
	local parent_bounds = {parent:bounds()}

	local parent_offset_x = -parent_bounds[1]
	local parent_offset_y = -parent_bounds[2]
	local parent_width = parent_bounds[3] - parent_bounds[1]
	local parent_height = parent_bounds[4] - parent_bounds[2]

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

	offset_x = offset_x or 0
	offset_y = offset_y or 0

	return self:location(left + offset_x - parent_offset_x, bottom + offset_y - parent_offset_y)
		:bounds(-offset_x, -offset_y, width - offset_x, height - offset_y)
		:invalidate()
end

function Widget.prototype:add_to(parent)
	return parent:add_child(self)
end

Widget.root = commands.get_root_widget
