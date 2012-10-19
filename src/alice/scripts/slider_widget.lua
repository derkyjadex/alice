-- slider_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = class(Widget, function(self, callback)
	Widget.init(self)

	local length
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

	function self.layout(self, left, width, right, bottom, height, top)
		Widget.prototype.layout(self, left, 20, right, bottom, height, top)

		local bounds = {self:bounds()}
		length = bounds[4] - bounds[2] - 20
	end
end)
