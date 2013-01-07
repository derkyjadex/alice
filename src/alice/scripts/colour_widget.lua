-- colour_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ColourWidget = Widget:derive(function(self)
	Widget.init(self)

	local callback = function() end
	local hue, sat, val = 0, 0, 1
	local red, green, blue
	local hue_handle, val_handle

	local function set_rgb()
		local c = sat * val
		local h_ = hue / (math.pi / 3)
		local d = c * (1 - math.abs(math.fmod(h_, 2) - 1))

		local r, g, b

		if (h_ < 1) then
			r, g, b = c, d, 0
		elseif (h_ < 2) then
			r, g, b = d, c, 0
		elseif (h_ < 3) then
			r, g, b = 0, c, d
		elseif (h_ < 4) then
			r, g, b = 0, d, c
		elseif (h_ < 5) then
			r, g, b = d, 0, c
		else
			r, g, b = c, 0, d
		end

		local m = val - c

		red = r + m
		green = g + m
		blue = b + m

		self:fill_colour(red, green, blue, 1)
		hue_handle:fill_colour(1 - red, 1 - green, 1 - blue, 1)
		val_handle:fill_colour(1 - red, 1 - green, 1 - blue, 1)

		callback(red, green, blue)
	end

	local function get_hue_handle_location()
		local x = 90 * sat * math.cos(hue - math.pi)
		local y = 90 * sat * math.sin(hue - math.pi)

		return x, y
	end

	local function get_val_handle_location()
		return 110, 180 * val - 90
	end

	local function hue_handle_drag(x, y)
		x, y = x / 90, y / 90
		hue = math.atan2(y, x) + math.pi
		sat = clamp(math.sqrt(x * x + y * y), 0, 1)
		set_rgb()

		return get_hue_handle_location()
	end

	local function set_hue_handle_location()
		hue_handle:location(get_hue_handle_location())
	end

	local function val_handle_drag(x, y)
		val = clamp((y + 90) / 180, 0, 1)
		set_rgb()

		return get_val_handle_location()
	end

	local function set_val_handle_location()
		val_handle:location(get_val_handle_location())
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

	set_rgb()
	hue_handle:location(get_hue_handle_location())
	val_handle:location(get_val_handle_location())

	function self:bind_change(new_callback)
		callback = new_callback

		return self
	end
end)

function ColourWidget.prototype:layout(left, width, right, bottom, height, top)
	return Widget.prototype.layout(self, left, 220, right, bottom, 200, top, 100, 100)
end
