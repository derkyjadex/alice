-- slider_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = class(Widget, function(self, x, y, length, callback)
	Widget.init(self)

	self:set_location(x, y)
	self:set_bounds(0, 0, 20, length + 20)
	self:set_fill_colour(0, 0, 0, 1)
	self:set_border_colour(1, 1, 1, 1)
	self:set_border_width(2)

	local handle = Widget()
	handle:set_bounds(0, 0, 20, 20)
	handle:set_fill_colour(1, 1, 1, 1)
	make_draggable(handle, nil, nil, function(x, y)
		callback(y / length)
	end)
	self:add_child(handle)
end)
