-- slider_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = Widget:derive(function(self)
	Widget.init(self)

	local length = 0
	local min, max = 0, 1
	local value = 0
	local value_binding = function() end

	self:fill_colour(0, 0, 0, 1)
		:border_colour(1, 1, 1, 1)
		:border_width(2)

	local handle = Widget():add_to(self)
		:bounds(0, 0, 20, 20)
		:fill_colour(1, 1, 1, 1)
	make_draggable(handle, nil, nil, function(x, y)
		value = min + (max - min) * (y / length)
		value_binding(value)
	end)

	local function update_location()
		local y = length * (value - min) / (max - min)
		handle:location(0, y):invalidate()
	end

	local function update_value(new_value)
		value = clamp(new_value, min, max)
		update_location()
	end

	self.range = make_accessor(
		function(_) return min, max end,
		function(_, new_min, new_max)
			min, max = new_min, new_max
			update_value(value)
		end)

	function self:bind_value(observable)
		value_binding = Binding(observable, update_value)

		return self
	end

	function self:layout(left, width, right, bottom, height, top)
		Widget.prototype.layout(self, left, 20, right, bottom, height, top)

		local bounds = {self:bounds()}
		length = bounds[4] - bounds[2] - 20

		update_location()

		return self
	end
end)
