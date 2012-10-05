-- model_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ModelWidget = class(Widget, function(self, x, y, width, height)
	Widget.init(self)

	self:location(x + width / 2, y + height / 2)
	self:bounds(-width / 2, -height / 2, width / 2, height / 2)
	self:border_width(0)
	self:fill_colour(0.2, 0.2, 0.2, 0.2)
	self:model_scale(100)
	self:grid_size(20, 20)
	self:grid_colour(0.4, 0.4, 0.4)
	self:bind_down(set_current, self, nil)

	self._model = Model()
	self._paths = {}
	self._handles = {}
	self._current = nil
	self._mid_handles = {}

	finish_changes(self)
end)

function ModelWidget.prototype.free(self)
	Widget.free(self)
	self._model:free()
end

function finish_changes(self)
	self:model(self._model)
	update_handles(self)
	update_mid_handles(self)
end

function update_handles(self)
	local pathIndex, pointIndex = 0, 0
	if self._current then
		pathIndex, pointIndex = self._current.pathIndex, self._current.pointIndex
	end
	self._current = nil

	for i, handle in ipairs(self._handles) do
		handle.widget:remove()
		handle.widget:free()
	end
	self._handles = {}

	for i, path in ipairs(self._paths) do
		local points = path:points()

		for j, point in ipairs(points) do
			local widget = Widget()
			widget:location(point[1] * 100, point[2] * 100)
			widget:bounds(-6, -6, 6, 6)
			widget:border_width(1)
			widget:fill_colour(0.9, 0.9, 0.9, 0.9)
			self:add_child(widget)

			local handle = {widget = widget, path = path, pathIndex = i, pointIndex = j}

			make_draggable(widget,
				function() set_current(self, handle) end,
				function() finish_changes(self) end,
				function(x, y) path:set_point(j, x / 100, y / 100) end)

			if i == pathIndex and j == pointIndex then
				set_current(self, handle)
			end

			table.insert(self._handles, handle)
		end
	end
end

function update_mid_handles(self)
	for i, handle in ipairs(self._mid_handles) do
		handle.widget:remove()
		handle.widget:free()
	end
	self._mid_handles = {}

	for i, path in ipairs(self._paths) do
		local points = path:points()

		for j = 1, #points - 1 do
			local p1 = points[j]
			local p2 = points[j + 1]

			local x = p1[1] + (p2[1] - p1[1]) / 2
			local y = p1[2] + (p2[2] - p1[2]) / 2

			local widget = Widget()
			widget:location(x * 100, y * 100)
			widget:bounds(-4, -4, 4, 4)
			widget:fill_colour(0.2, 0.5, 0.9, 0.9)
			self:add_child(widget)

			local handle = {widget = widget, path = path, pathIndex = i, i1 = j, i2 = j + 1}

			widget:bind_down(add_point, self, path, j + 1, x, y)

			table.insert(self._mid_handles, handle)
		end
	end
end

function set_current(self, handle)
	if self._current then
		self._current.widget:fill_colour(0.9, 0.9, 0.9, 0.9)
	end

	self._current = handle

	if self._current then
		handle.widget:fill_colour(0.9, 0.5, 0.5, 0.9)
	end
end

function add_point(self, path, index, x, y)
	path:add_point(index, x, y)
	finish_changes(self)
end

function ModelWidget.prototype.load(self, filename)
	self._model:load(filename)
	self._paths = self._model:paths()
	finish_changes(self)
end

function ModelWidget.prototype.save(self, filename)
	self._model:save(filename)
end

function ModelWidget.prototype.add_path(self)
	self._model:add_path(0, -0.2, 0, 0.2, 0)
	self._paths = self._model:paths()
	finish_changes(self)
end

function ModelWidget.prototype.remove_path(self)
	if self._current then
		self._model:remove_path(self._current.pathIndex)
		self._paths = self._model:paths()
		finish_changes(self)
	end
end

function ModelWidget.prototype.remove_point(self)
	if self._current then
		local path = self._current.path
		local i = self._current.pointIndex
		local num_points = #path:points()

		if i > 1 and i < num_points then
			path:remove_point(self._current.pointIndex)
			finish_changes(self)
		end
	end
end

function ModelWidget.prototype.get_path_colour(self)
	if self._current then
		return self._current.path:colour()
	else
		return 0, 0, 0
	end
end

function ModelWidget.prototype.set_path_colour(self, r, g, b)
	if self._current then
		self._current.path:colour(r, g, b)
		finish_changes(self)
	end
end
