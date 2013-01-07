-- toolbar.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

Toolbar = Widget:derive(function(self)
	Widget.init(self)

	self:fill_colour(1, 1, 1, 0.8)
	self._nextX = 5
end)

function Toolbar.prototype.add_button(self, r, g, b)
	local button = Widget():add_to(self)
		:location(self._nextX, 5)
		:bounds(0, 0, 30, 30)
		:fill_colour(r, g, b, 0.9)

	self._nextX = self._nextX + 35

	return button
end

function Toolbar.prototype:add_spacer()
	self._nextX = self._nextX + 10
end

function Toolbar.prototype:layout(left, width, right, bottom, height, top)
	return Widget.prototype.layout(self, left, self._nextX, right, bottom, 40, top)
end
