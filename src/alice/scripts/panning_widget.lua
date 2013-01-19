-- slider_widget.lua
-- Copyright (c) 2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

PanningWidget = Widget:derive(function(self)
	Widget.init(self)

	local value_binding = function() end
	local mouse_x, mouse_y
	local scale = 1
	local x, y = 0, 0

	self:fill_colour(0, 0, 0, 1)
		:border_colour(1, 1, 1, 1)
		:border_width(2)
		:bind_down(function()
			mouse_x, mouse_y = self:grab_mouse()
		end)
		:bind_up(function()
			self:release_mouse(mouse_x, mouse_y)
		end)
		:bind_motion(function(_, x_, y_)
			x, y = x + x_ / scale, y + y_ / scale
			value_binding(x, y)
		end)

	self.scale = make_accessor(
		function(_) return 1 end,
		function(_, new_scale)
			scale = new_scale
			return self
		end)

	function self:bind_value(observable)
		value_binding = binding(observable, function(new_x, new_y)
			x, y = new_x, new_y
		end)

		return self
	end

	function self:layout(left, width, right, bottom, height, top)
		return Widget.prototype.layout(self, left, 20, right, bottom, 20, top)
	end
end)
