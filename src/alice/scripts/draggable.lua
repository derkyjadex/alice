-- draggable.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local drag_offset_x, drag_offset_y

local function start_drag(widget, callback)
	local widget_x, widget_y = widget:location()
	local mouse_x, mouse_y = widget:grab_mouse()

	drag_offset_x = mouse_x - widget_x
	drag_offset_y = mouse_y - widget_y

	if callback then callback() end
end

local function end_drag(widget, callback)
	local widget_x, widget_y = widget:location()
	local mouse_x = widget_x + drag_offset_x
	local mouse_y = widget_y + drag_offset_y

	widget:release_mouse(mouse_x, mouse_y)

	if callback then callback() end
end

function make_draggable(widget, on_down, on_up, on_motion)
	local parent

	function drag(widget, motion_x, motion_y)
		local widget_x, widget_y = widget:location()
		local x = widget_x + motion_x
		local y = widget_y + motion_y

		local min_x, min_y, max_x, max_y = widget:bounds()
		parent = parent or widget:parent()
		local parent_min_x, parent_min_y, parent_max_x, parent_max_y = parent:bounds()

		x = clamp(x, parent_min_x - min_x, parent_max_x - max_x)
		y = clamp(y, parent_min_y - min_y, parent_max_y - max_y)

		if on_motion then
			local new_x, new_y = on_motion(x, y)
			if new_x ~= nil and new_y ~= nil then
				x, y = new_x, new_y
			end
		end

		widget:location(x, y)
	end

	widget:bind_down(start_drag, widget, on_down)
	widget:bind_up(end_drag, widget, on_up)
	widget:bind_motion(drag)
end
