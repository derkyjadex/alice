-- toolbar.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

Toolbar = class(Widget, function(self, x, y)
	Widget.init(self)

	self:set_location(x, y)
	self:set_bounds(0, 0, 5, 40)
	self:set_fill_colour(1, 1, 1, 0.8)

	self._nextX = 5
end)

function Toolbar.prototype.add_button(self, r, g, b)
	local button = Widget()
	button:set_location(self._nextX, 5)
	button:set_bounds(0, 0, 30, 30)
	button:set_fill_colour(r, g, b, 0.9)
	self:add_child(button)

	self._nextX = self._nextX + 35
	self:set_bounds(0, 0, self._nextX, 40)

	return button
end

function Toolbar.prototype.add_spacer(self)
	self._nextX = self._nextX + 10
end
