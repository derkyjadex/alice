-- model_widget.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local function get_transform(self)
	local scale = self._scale_binding()
	local pan_x, pan_y = self._pan_binding()
	return scale, pan_x, pan_y
end

local function select_path(self, x, y)
	local scale, pan_x, pan_y = get_transform(self)
	x, y = (x / scale) - pan_x, (y / scale) - pan_y

	local path_vms = self._model:paths()
	for i=#path_vms,1,-1 do
		local path_vm = path_vms[i]
		if path_vm:path():hit_test(x, y) then
			path_vm:select()
			return
		end
	end

	self._model.selected_path(nil)
end

ModelWidget = Widget:derive(function(self)
	Widget.init(self)

	self:border_width(2)
		:fill_colour(0.2, 0.2, 0.2, 1)
		:model_scale(1)
		:grid_size(20, 20)
		:grid_colour(0.4, 0.4, 0.4)
		:bind_down(select_path)

	self._model = nil
	self._paths = {}
	self._scale_binding = function() return 1 end
	self._centre_offset = {0, 0}
	self._pan_binding = function() return 0, 0 end
end)

local function update_model(self)
	self:model(self._model:model())
end

local function transform_handle(handle, scale, pan_x, pan_y)
	local x, y = handle.point.location()
	handle:location((x + pan_x) * scale, (y + pan_y) * scale)
end

local function update_transform(self)
	local offset = self._centre_offset
	local scale, pan_x, pan_y = get_transform(self)

	for _, path in ipairs(self._paths) do
		for _, handle in ipairs(path.handles) do
			transform_handle(handle, scale, pan_x, pan_y)
		end
		for _, handle in ipairs(path.mid_handles) do
			transform_handle(handle, scale, pan_x, pan_y)
		end
	end

	self:model_scale(scale)
		:model_location(pan_x * scale, pan_y * scale)
		:grid_size(scale * 20, scale * 20)
		:grid_offset(pan_x * scale + offset[1], pan_y * scale + offset[2])
		:invalidate()
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

local function move_point(self, point_vm, x, y)
	local scale, pan_x, pan_y = get_transform(self)
	point_vm.location((x / scale) - pan_x, (y / scale) - pan_y)
	update_model(self)
end

local function insert_handle(self, path, i, point_vm)
	local handle = Widget():add_to(path.group)
		:bounds(-6, -6, 6, 6)
		:fill_colour(1.0, 1.0, 1.0, 1.0)
		:border_width(1)
		:border_colour(0.1, 0.1, 0.1, 1.0)

	handle.point = point_vm

	Binding(point_vm.location, function(x, y)
		local scale, pan_x, pan_y = get_transform(self)
		transform_handle(handle, scale, pan_x, pan_y)
		update_model(self)
	end)

	make_draggable(handle, nil, nil,
		function(x, y) move_point(self, point_vm, x, y) end)

	table.insert(path.handles, i, handle)
	update_model(self)
end

local function insert_mid_handle(self, path, i, point_vm)
	local handle = Widget():add_to(path.group)
		:bounds(-4, -4, 4, 4)
		:fill_colour(0.2, 0.5, 0.9, 1.0)
		:border_colour(0.9, 0.9, 0.9, 1.0)
		:border_width(1)
		:bind_down(function()
			point_vm:subdivide()
		end)

	handle.point = point_vm

	Binding(point_vm.location, function(x, y)
		local scale, pan_x, pan_y = get_transform(self)
		transform_handle(handle, scale, pan_x, pan_y)
		update_model(self)
	end)

	table.insert(path.mid_handles, handle)
	update_model(self)
end

local function remove_handle(self, path, i, point_vm)
	path.handles[i]:remove()
	table.remove(path.handles, i)
	update_model(self)
end

local function remove_mid_handle(self, path, i, point_vm)
	path.mid_handles[i]:remove()
	table.remove(path.mid_handles, i)
	update_model(self)
end

local function clear_handles(self, path)
	for i, handle in ipairs(path.handles) do
		handle:remove()
	end

	path.handles = {}
	update_model(self)
end

local function clear_mid_handles(self, path)
	for i, handle in ipairs(path.mid_handles) do
		handle:remove()
	end

	path.mid_handles = {}
	update_model(self)
end

local function insert_path(self, i, path_vm)
	local points = path_vm:points()
	local mid_points = path_vm:mid_points()

	local path = {
		group = Widget():add_to(self)
			:pass_through(true)
			:bounds(self:bounds()),
		handles = {},
		mid_handles = {}
	}

	path.group:bind_property('visible', path_vm.selected)

	path_vm.colour.changed:add(function() update_model(self) end)

	points.inserted:add(function(i, point) insert_handle(self, path, i, point) end)
	points.removed:add(function(i, point) remove_handle(self, path, i, point) end)
	points.updated:add(function(i, point)
		remove_handle(self, path, i, point)
		insert_handle(self, path, i, point)
	end)
	points.cleared:add(function() clear_handles(self, path) end)

	mid_points.inserted:add(function(i, point) insert_mid_handle(self, path, i, point) end)
	mid_points.removed:add(function(i, point) remove_mid_handle(self, path, i, point) end)
	mid_points.updated:add(function(i, point)
		remove_mid_handle(self, path, i, point)
		insert_mid_handle(self, path, i, point)
	end)
	mid_points.cleared:add(function() clear_mid_handles(self, path) end)

	for i, point in ipairs(points) do
		insert_handle(self, path, i, point)
	end

	for i, point in ipairs(mid_points) do
		insert_mid_handle(self, path, i, point)
	end

	table.insert(self._paths, i, path)
	update_model(self)
end

local function remove_path(self, i, path_vm)
	local path = self._paths[i]
	clear_handles(self, path)
	clear_mid_handles(self, path)
	path.group:remove()

	table.remove(self._paths, i)
	update_model(self)
end

local function clear_paths(self)
	for i, path in ipairs(self._paths) do
		clear_handles(self, path)
		clear_mid_handles(self, path)
		path.group:remove()
	end

	self._paths = {}
	update_model(self)
end

function ModelWidget.prototype:bind_model(model)
	self._model = model

	local paths = model:paths()
	paths.inserted:add(function(i, path) insert_path(self, i, path) end)
	paths.removed:add(function(i, path) remove_path(self, i, path) end)
	paths.updated:add(function(i, path)
		remove_path(self, i, path)
		insert_path(self, i, path)
	end)
	paths.cleared:add(function() clear_paths(self) end)

	clear_paths(self)

	for i, path in ipairs(paths) do
		insert_path(i, path)
	end

	update_model(self)

	return self
end

function ModelWidget.prototype:bind_scale(observable)
	self._scale_binding = Binding(observable, function()
		update_transform(self)
	end)

	return self
end

function ModelWidget.prototype:bind_pan(observable)
	self._pan_binding = Binding(observable, function()
		update_transform(self)
	end)

	return self
end
