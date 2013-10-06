-- model_view_model.lua
-- Copyright (c) 2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local function smooth_point(mid_point_vm)
	if mid_point_vm then
		local point = mid_point_vm:point()
		local x1, y1 = point:location()
		local x2, y2 = point:next():location()
		local bias = point:curve_bias()

		local x = (1 - bias) * x1 + bias * x2
		local y = (1 - bias) * y1 + bias * y2

		mid_point_vm:location(x, y)
	end
end

local function remove_item(array, item)
	if item then
		for i, current in ipairs(array) do
			if current == item then
				array:remove(i)
				return
			end
		end
	end
end

local function ModelMidPointViewModel(path_vm, point_vm)
	local self = {
		location = Property(0, 0)
	}

	function self:point() return point_vm end
	function self:subdivide() path_vm:subdivide_mid_point(self) end

	return self
end

local function ModelPointViewModel(path_vm, p, index)
	local self = {
		index = Property(index),
		next = Property(),
		prev = Property(),
		location = Property(p[1], p[2]),
		curve_bias = Property(p[3])
	}

	local mid_point_vm = nil

	function self:path() return path_vm end
	function self:remove() path_vm:remove_point(self) end
	function self:mid_point() return mid_point_vm end

	function self:update_mid_point()
		local need_mid_point = self:curve_bias() ~= 0 and self:next():curve_bias() ~= 0

		if need_mid_point and not mid_point_vm then
			mid_point_vm = ModelMidPointViewModel(path_vm, self)
			path_vm:mid_points():insert(mid_point_vm)

		elseif not need_mid_point and mid_point_vm then
			remove_item(path_vm:mid_points(), mid_point_vm)
			mid_point_vm = nil
		end

		smooth_point(mid_point_vm)
	end

	self.location.changed:add(function(x, y)
		path_vm.path():set_point(self:index(), x, y, self:curve_bias())

		smooth_point(mid_point_vm)
		smooth_point(self:prev():mid_point())
	end)

	self.curve_bias.changed:add(function(curve_bias)
		local x, y = self:location()
		path_vm.path():set_point(self:index(), x, y, curve_bias)

		self:update_mid_point()
		self:prev():update_mid_point()
	end)

	return self
end

local function ModelPathViewModel(model_vm, path, index)
	local point_vms = ObservableArray()
	local mid_point_vms = ObservableArray()

	local self = {
		index = Property(index),
		colour = Property(path:colour())
	}

	self.colour.changed:add(function(r, g, b)
		path:colour(r, g, b)
	end)

	function self:path() return path end
	function self:points() return point_vms end
	function self:mid_points() return mid_point_vms end

	function self:remove_point(point_vm)
		local p1, p2, p3 = point_vm:prev(), point_vm, point_vm:next()

		if p1 == p3 or p1:prev() == p3 then
			return
		end

		path:remove_point(p2:index())

		p1:next(p3)
		p3:prev(p1)

		remove_item(point_vms, p2)
		for i = p2:index(), #point_vms do
			point_vms[i]:index(i)
		end

		remove_item(mid_point_vms, p2:mid_point())

		p1:update_mid_point()
	end

	function self:subdivide_mid_point(mid_point_vm)
		local p1 = mid_point_vm:point()
		local p3 = p1:next()
		local i2 = p1:index() + 1
		local x, y = mid_point_vm:location()

		path:add_point(i2, x, y, 0.5)

		local p2 = ModelPointViewModel(self, {x, y, 0.5}, i2)

		p1:next(p2)
		p2:prev(p1)
		p2:next(p3)
		p3:prev(p2)

		point_vms:insert(i2, p2)
		for i = i2, #point_vms do
			point_vms[i]:index(i)
		end

		p1:update_mid_point()
		p2:update_mid_point()
	end

	local function rebuild_point_vms()
		point_vms:clear()
		mid_point_vms:clear()

		for i, p in ipairs(path:points()) do
			local prev = point_vms[#point_vms]
			local point_vm = ModelPointViewModel(self, p, i)

			point_vms:insert(point_vm)

			if prev then
				prev:next(point_vm)
				point_vm:prev(prev)
				prev:update_mid_point()
			end
		end

		point_vms[#point_vms]:next(point_vms[1])
		point_vms[1]:prev(point_vms[#point_vms])
		point_vms[#point_vms]:update_mid_point()
	end

	rebuild_point_vms()

	return self
end

function ModelViewModel(model)
	model = model or Model()
	local path_vms = ObservableArray()

	local self = {}

	local function rebuild_path_vms()
		path_vms:clear()

		for i, path in ipairs(model:paths()) do
			path_vms:insert(ModelPathViewModel(self, path, i))
		end
	end

	function self:model() return model end
	function self:paths() return path_vms end

	function self:add_path()
		local path = model:add_path(0, 20, -20, 0.5, 20, 20, 0.5)
		path:add_point(0, -20, 20, 0.5)
		path:add_point(0, -20, -20, 0.5)

		local path_vm = ModelPathViewModel(self, path, #path_vms + 1)
		path_vms:insert(path_vm)

		return path_vm
	end

	function self:remove_path(path_vm)
		local index = path_vm:index()
		model:remove_path(index)
		path_vms:remove(index)

		for i = index, #path_vms do
			path_vms[i]:index(i)
		end
	end

	function self:load(filename)
		model:load(filename)
		rebuild_path_vms()
	end

	function self:save(filename)
		model:save(filename)
	end

	rebuild_path_vms()

	return self
end

function ModelContextViewModel(model)
	local self = ModelViewModel(model)
	self.selected_path = Property()
	self.current_colour = Property(1, 1, 1)
	local paths = self:paths()

	self.selected_path.changed:add(function(path_vm)
		if path_vm then
			self:current_colour(path_vm:colour())
		end
	end)
	self.current_colour.changed:add(function(r, g, b)
		local path_vm = self:selected_path()
		if path_vm then
			path_vm:colour(r, g, b)
		end
	end)

	paths.inserted:add(function(i, path_vm)
		path_vm.selected = Computed(self.selected_path,
			function(selected) return path_vm == selected end)

		local model_vm = self
		function path_vm:select()
			model_vm:selected_path(self)
		end
	end)
	paths.removed:add(function(i, path_vm)
		if path_vm == self:selected_path() then
			self:selected_path(nil)
		end
	end)
	paths.cleared:add(function() self:selected_path(nil) end)

	local base_add_path = self.add_path
	function self:add_path()
		local path_vm = base_add_path()
		path_vm:colour(self:current_colour())

		return path_vm
	end

	local base_remove_path = self.remove_path
	function self:remove_path(path_vm)
		path_vm = path_vm or self:selected_path()
		if not path_vm then
			return
		end

		if self:selected_path() == path_vm then
			self:selected_path(nil)
		end

		base_remove_path(self, path_vm)
	end

	return self
end
