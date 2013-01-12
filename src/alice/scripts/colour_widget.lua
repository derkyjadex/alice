-- colour_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local function hsv_to_rgb(h, s, v)
	local c = s * v
	local h_ = h / (math.pi / 3)
	local d = c * (1 - math.abs(math.fmod(h_, 2) - 1))
	local m = v - c

	local r, g, b

	if h_ < 1 then
		r, g, b = c, d, 0
	elseif h_ < 2 then
		r, g, b = d, c, 0
	elseif h_ < 3 then
		r, g, b = 0, c, d
	elseif h_ < 4 then
		r, g, b = 0, d, c
	elseif h_ < 5 then
		r, g, b = d, 0, c
	else
		r, g, b = c, 0, d
	end

	return r + m, g + m, b + m
end

local function rgb_to_hsv(r, g, b)
	local v = math.max(r, g, b)
	local m = math.min(r, g, b)
	local c = v - m

	local s
	if v == 0 then
		s = 0
	else
		s = c / v
	end

	local h_

	if c == 0 then
		h_ = 0
	elseif v == r then
		h_ = math.fmod((g - b) / c, 6)
	elseif v == g then
		h_ = ((b - r) / c) + 2
	elseif v == b then
		h_ = ((r - g) / c) + 4
	end

	return h_ * (math.pi / 3), s, v
end

ColourWidget = Widget:derive(function(self)
	Widget.init(self)

	local hue, sat, val = 0, 0, 1
	local red, green, blue = hsv_to_rgb(hue, sat, val)
	local value_binding = function() end
	local hue_handle, val_handle

	local function get_hue_handle_location()
		local x = 90 * sat * math.cos(hue - math.pi)
		local y = 90 * sat * math.sin(hue - math.pi)

		return x, y
	end

	local function get_val_handle_location()
		return 110, 180 * val - 90
	end

	local function update_fill_colours()
		self:fill_colour(red, green, blue, 1)
		hue_handle:fill_colour(1 - red, 1 - green, 1 - blue, 1)
		val_handle:fill_colour(1 - red, 1 - green, 1 - blue, 1)
	end

	local function update_value(r, g, b)
		red, green, blue = clamp(r, 0, 1), clamp(g, 0, 1), clamp(b, 0, 1)
		hue, sat, val = rgb_to_hsv(red, green, blue)

		update_fill_colours()
		hue_handle:location(get_hue_handle_location())
		val_handle:location(get_val_handle_location())
	end

	local function update_from_hsv()
		red, green, blue = hsv_to_rgb(hue, sat, val)
		update_fill_colours()

		value_binding(red, green, blue)
	end

	local function hue_handle_drag(x, y)
		x, y = x / 90, y / 90
		hue = math.atan2(y, x) + math.pi
		sat = clamp(math.sqrt(x * x + y * y), 0, 1)
		update_from_hsv()

		return get_hue_handle_location()
	end

	local function val_handle_drag(x, y)
		val = clamp((y + 90) / 180, 0, 1)
		update_from_hsv()

		return get_val_handle_location()
	end

	self:border_width(2)

	hue_handle = Widget():add_to(self)
		:bounds(-10, -10, 10, 10)
		:fill_colour(0.5, 0.5, 0.5, 0.8)
	make_draggable(hue_handle, nil, nil, hue_handle_drag)

	val_handle = Widget():add_to(self)
		:bounds(-10, -10, 10, 10)
		:fill_colour(0.5, 0.5, 0.5, 0.8)
	make_draggable(val_handle, nil, nil, val_handle_drag)

	function self:bind_value(observable)
		value_binding = binding(observable, update_value)

		return self
	end
end)

function ColourWidget.prototype:layout(left, width, right, bottom, height, top)
	return Widget.prototype.layout(self, left, 220, right, bottom, 200, top, 100, 100)
end
