-- widget.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local host = require 'host'
local widget = require 'widget'

Widget = wrap('widget')

local vars = {
	'visible', 'pass_through', 'location', 'bounds', 'fill_colour',
	'border_colour', 'border_width',
	'grid_size', 'grid_offset', 'grid_colour',
	'model_location', 'model_scale',
	'text', 'text_colour', 'text_size', 'text_location'}

for i,var in ipairs(vars) do
	Widget.prototype[var] = make_var_accessor('widget.' .. var)
end

Widget.prototype.next = widget.get_next
Widget.prototype.prev = widget.get_prev
Widget.prototype.parent = widget.get_parent
Widget.prototype.first_child = widget.get_first_child
Widget.prototype.model = widget.set_model
Widget.prototype.add_child = widget.add_child
Widget.prototype.add_sibling = widget.add_sibling
Widget.prototype.remove = widget.remove
Widget.prototype.invalidate = widget.invalidate
Widget.prototype.bind_up = widget.bind_up
Widget.prototype.bind_down = widget.bind_down
Widget.prototype.bind_motion = widget.bind_motion
Widget.prototype.bind_key = widget.bind_key
Widget.prototype.bind_text = widget.bind_text
Widget.prototype.bind_keyboard_lost = widget.bind_keyboard_lost
Widget.prototype.grab_mouse = host.grab_mouse
Widget.prototype.release_mouse = function(_, x, y) return host.release_mouse(x, y) end
Widget.prototype.grab_keyboard = host.grab_keyboard
Widget.prototype.release_keyboard = host.release_keyboard

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

function Widget.prototype:bind_property(name, observable)
	local var = self[name]
	observable.changed:add(function(...)
		var(self, ...)
		self:invalidate()
	end)

	var(self, observable())
	self:invalidate()

	return self
end

Widget.root = host.get_root_widget
