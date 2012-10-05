-- slider_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = class(Widget, function(self, x, y, length, callback)
	Widget.init(self)

	self:location(x, y)
	self:bounds(0, 0, 20, length + 20)
	self:fill_colour(0, 0, 0, 1)
	self:border_colour(1, 1, 1, 1)
	self:border_width(2)

	local handle = Widget()
	handle:bounds(0, 0, 20, 20)
	handle:fill_colour(1, 1, 1, 1)
	make_draggable(handle, nil, nil, function(x, y)
		callback(y / length)
	end)
	self:add_child(handle)
end)
