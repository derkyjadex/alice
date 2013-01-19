-- slider_widget.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

SliderWidget = Widget:derive(function(self)
	Widget.init(self)

	local length = 0
	local scale_type = SliderWidget.LinearScale
	local scale = scale_type(0, 1)
	local value = 0
	local value_binding = function() end

	self:fill_colour(0, 0, 0, 1)
		:border_colour(1, 1, 1, 1)
		:border_width(2)

	local handle = Widget():add_to(self)
		:bounds(0, 0, 20, 20)
		:fill_colour(1, 1, 1, 1)
	make_draggable(handle, nil, nil, function(x, y)
		value = scale.value(y / length)
		value_binding(value)
	end)

	local function update_location()
		local y = length * scale.invert(value)
		handle:location(0, y):invalidate()
	end

	local function update_value(new_value)
		value = scale.clamp(new_value)
		update_location()
	end

	function self:range(min, max)
		scale = scale_type(min, max)
		update_value(value)

		return self
	end

	function self:scale_type(type)
		scale_type = type
		scale = scale_type(scale.range())
		update_value(value)

		return self
	end

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

local function Scale(min, max, value, invert)
	local range = max - min

	return {
		range = function() return min, max end,
		clamp = function(x) return clamp(x, min, max) end,
		value = value,
		invert = invert
	}
end

function SliderWidget.LinearScale(min, max)
	local range = max - min

	return Scale(min, max,
		function(x) return min + range * x end,
		function(y) return (y - min) / range end)
end

function SliderWidget.LogScale(min, max)
	local range = math.log(max / min)

	return Scale(min, max,
		function(x) return min * math.exp(range * x) end,
		function(y) return math.log(y / min) / range end)
end
