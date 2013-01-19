-- model_widget.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local finish_changes, set_current, add_point

ModelWidget = Widget:derive(function(self)
	Widget.init(self)

	self:border_width(2)
		:fill_colour(0.2, 0.2, 0.2, 1)
		:model_scale(1)
		:grid_size(20, 20)
		:grid_colour(0.4, 0.4, 0.4)
		:bind_down(function() set_current(self, nil) end)

	self._model = Model()
	self._handles = {}
	self._current = nil
	self._mid_handles = {}
	self._path_colour_binding = function() end
	self._scale_binding = function() return 1 end
	self._centre_offset = {0, 0}
	self._pan_binding = function() return 0, 0 end

	finish_changes(self, true)
end)

local function rebuild_handles(self)
	local path_index, point_index = 0, 0
	if self._current then
		path_index, point_index = self._current.path_index, self._current.point_index
	end
	self._current = nil

	for _, handle in pairs(self._handles) do
		handle.widget:remove()
	end
	self._handles = {}

	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	for i, path in ipairs(self._model:paths()) do
		local points = path:points()

		for j, point in ipairs(points) do
			local widget = Widget():add_to(self)
				:location((point[1] + pan_x) * scale, (point[2] + pan_y) * scale)
				:bounds(-6, -6, 6, 6)
				:border_width(1)
				:fill_colour(0.9, 0.9, 0.9, 0.9)

			local handle = {widget = widget, path = path, path_index = i, point_index = j}

			make_draggable(widget,
				function() set_current(self, handle) end,
				function() finish_changes(self, false) end,
				function(x, y) move_point(self, path, j, x, y) end)

			if i == path_index and j == point_index then
				set_current(self, handle)
			end

			table.insert(self._handles, handle)
		end
	end
end

local function rebuild_mid_handles(self)
	for _, handle in pairs(self._mid_handles) do
		handle.widget:remove()
	end
	self._mid_handles = {}

	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	for i, path in ipairs(self._model:paths()) do
		local points = path:points()

		for j = 1, #points - 1 do
			local p1 = points[j]
			local p2 = points[j + 1]

			local x = p1[1] + (p2[1] - p1[1]) / 2
			local y = p1[2] + (p2[2] - p1[2]) / 2

			local widget = Widget():add_to(self)
				:location((x + pan_x) * scale, (y + pan_y) * scale)
				:bounds(-4, -4, 4, 4)
				:fill_colour(0.2, 0.5, 0.9, 0.9)

			local handle = {
				widget = widget,
				path = path, path_index = i,
				i1 = j, i2 = j + 1,
				x = x, y = y
			}

			widget:bind_down(function()
				add_point(self, path, j + 1, handle.x, handle.y)
			end)

			table.insert(self._mid_handles, handle)
		end
	end
end

local function update_handles(self)
	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	for _, handle in pairs(self._handles) do
		local point = handle.path:points()[handle.point_index]
		handle.widget:location((point[1] + pan_x) * scale, (point[2] + pan_y) * scale)
			:invalidate()
	end
end

local function update_mid_handles(self)
	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	for _, handle in pairs(self._mid_handles) do
		local points = handle.path:points()
		local p1 = points[handle.i1]
		local p2 = points[handle.i2]
		local x = p1[1] + (p2[1] - p1[1]) / 2
		local y = p1[2] + (p2[2] - p1[2]) / 2

		handle.widget:location((x + pan_x) * scale, (y + pan_y) * scale)
			:invalidate()
		handle.x = x
		handle.y = y
	end
end

finish_changes = function(self, rebuild)
	self:model(self._model)

	if rebuild then
		rebuild_handles(self)
		rebuild_mid_handles(self)
	else
		update_handles(self)
		update_mid_handles(self)
	end
end

set_current = function(self, handle)
	if self._current then
		self._current.widget:fill_colour(0.9, 0.9, 0.9, 0.9)
	end

	self._current = handle

	if self._current then
		handle.widget:fill_colour(0.9, 0.5, 0.5, 0.9)

		self._path_colour_binding(self._current.path:colour())
	end
end

add_point = function(self, path, index, x, y)
	path:add_point(index, x, y)
	finish_changes(self, true)
end

move_point = function(self, path, index, x, y)
	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	path:set_point(index, (x / scale) - pan_x, (y / scale) - pan_y)
end

local function update_transform(self)
	local offset = self._centre_offset
	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()

	self:model_scale(scale)
		:model_location(pan_x * scale, pan_y * scale)
		:grid_size(scale * 20, scale * 20)
		:grid_offset(pan_x * scale + offset[1], pan_y * scale + offset[2])
		:invalidate()

	update_handles(self)
	update_mid_handles(self)
end

function ModelWidget.prototype:layout(left, width, right, bottom, height, top)
	Widget.prototype.layout(self, left, width, right, bottom, height, top)

	local bounds = {self:bounds()}

	self._centre_offset = {
		(bounds[3] - bounds[1]) / 2,
		(bounds[4] - bounds[2]) / 2
	}

	update_transform(self)

	return Widget.prototype.layout(self,
		left, width, right,
		bottom, height, top,
		(bounds[3] - bounds[1]) / 2, (bounds[4] - bounds[2]) / 2)
end

function ModelWidget.prototype:load(filename)
	self._model:load(filename)
	finish_changes(self, true)
end

function ModelWidget.prototype:save(filename)
	self._model:save(filename)
end

function ModelWidget.prototype:add_path()
	self._model:add_path(0, -20, 0, 20, 0)
		:colour(self._path_colour_binding())
	finish_changes(self, true)
end

function ModelWidget.prototype:remove_path()
	if self._current then
		self._model:remove_path(self._current.path_index)
		finish_changes(self, true)
	end
end

function ModelWidget.prototype:remove_point()
	if self._current then
		local path = self._current.path
		local i = self._current.point_index
		local num_points = #path:points()

		if i > 1 and i < num_points then
			path:remove_point(self._current.point_index)
			finish_changes(self, true)
		end
	end
end

function ModelWidget.prototype:bind_path_colour(observable)
	self._path_colour_binding = binding(observable, function(r, g, b)
		if self._current then
			self._current.path:colour(r, g, b)
			finish_changes(self, false)
		end
	end)

	return self
end

function ModelWidget.prototype:bind_scale(observable)
	self._scale_binding = binding(observable, function()
		update_transform(self)
	end)

	return self
end

function ModelWidget.prototype:bind_pan(observable)
	self._pan_binding = binding(observable, function()
		update_transform(self)
	end)

	return self
end
