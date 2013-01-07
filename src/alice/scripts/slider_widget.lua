-- slider_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = Widget:derive(function(self)
	Widget.init(self)

	local callback = function() end
	local length
	self:fill_colour(0, 0, 0, 1)
		:border_colour(1, 1, 1, 1)
		:border_width(2)

	local handle = Widget():add_to(self)
		:bounds(0, 0, 20, 20)
		:fill_colour(1, 1, 1, 1)
	make_draggable(handle, nil, nil, function(x, y)
		callback(y / length)
	end)

	function self:bind_change(new_callback)
		callback = new_callback

		return self
	end

	function self:layout(left, width, right, bottom, height, top)
		Widget.prototype.layout(self, left, 20, right, bottom, height, top)

		local bounds = {self:bounds()}
		length = bounds[4] - bounds[2] - 20

		return self
	end
end)
