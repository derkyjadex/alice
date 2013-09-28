-- model_view_model.lua
-- Copyright (c) 2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local function smooth_point(mid_point_vm)
	if mid_point_vm then
		local point = mid_point_vm:point()
		local x1, y1 = point.location()
		local x2, y2 = point.next.location()

		local x = 0.5 * x1 + 0.5 * x2
		local y = 0.5 * y1 + 0.5 * y2

		mid_point_vm.location(x, y)
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
		location = Observable(0, 0)
	}

	function self:point() return point_vm end
	function self:subdivide() path_vm:subdivide_mid_point(self) end

	return self
end

local function ModelPointViewModel(path_vm, p, index)
	local self = {
		index = index,
		location = Observable(p[1], p[2]),
		on_curve = Observable(p[3])
	}

	local mid_point_vm = nil

	function self:path() return path_vm end
	function self:remove() path_vm:remove_point(self) end
	function self:mid_point() return mid_point_vm end

	function self:update_mid_point()
		local need_mid_point = not self.on_curve() and not self.next.on_curve()

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
		path_vm.path():set_point(self.index, x, y, self.on_curve())

		smooth_point(mid_point_vm)
		smooth_point(self.prev.mid_point())
	end)

	self.on_curve.changed:add(function(on_curve)
		local x, y = self.location()
		path_vm.path():set_point(self.index, x, y, on_curve)

		self:update_mid_point()
		self.prev:update_mid_point()
	end)

	return self
end

local function ModelPathViewModel(model_vm, path, index)
	local point_vms = ObservableArray()
	local mid_point_vms = ObservableArray()

	local self = {
		index = index,
		colour = Observable(path:colour())
	}

	self.colour.changed:add(function(r, g, b)
		path:colour(r, g, b)
	end)

	function self:path() return path end
	function self:points() return point_vms end
	function self:mid_points() return mid_point_vms end

	function self:remove_point(point_vm)
		local p1, p2, p3 = point_vm.prev, point_vm, point_vm.next

		if p1 == p3 or p1.prev == p3 then
			return
		end

		path:remove_point(p2.index)

		p1.next = p3
		p3.prev = p1

		remove_item(point_vms, p2)
		for i = p2.index, #point_vms do
			point_vms[i].index = i
		end

		remove_item(mid_point_vms, p2:mid_point())

		p1:update_mid_point()
	end

	function self:subdivide_mid_point(mid_point_vm)
		local p1 = mid_point_vm:point()
		local p3 = p1.next
		local i2 = p1.index + 1
		local x, y = mid_point_vm.location()

		path:add_point(i2, x, y, false)

		local p2 = ModelPointViewModel(self, {x, y, false}, i2)

		p1.next = p2
		p2.prev, p2.next = p1, p3
		p3.prev = p2

		point_vms:insert(i2, p2)
		for i = i2, #point_vms do
			point_vms[i].index = i
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
				prev.next = point_vm
				point_vm.prev = prev
				prev:update_mid_point()
			end
		end

		point_vms[#point_vms].next = point_vms[1]
		point_vms[1].prev = point_vms[#point_vms]
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
		local path = model:add_path(0, 20, -20, false, 20, 20, false)
		path:add_point(0, -20, 20, false)
		path:add_point(0, -20, -20, false)

		local path_vm = ModelPathViewModel(self, path, #path_vms + 1)
		path_vms:insert(path_vm)

		return path_vm
	end

	function self:remove_path(path_vm)
		local index = path_vm.index
		model:remove_path(index)
		path_vms:remove(index)

		for i = index, #path_vms do
			path_vms[i].index = i
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
	local selected_path = Observable()
	local current_colour = Observable(1, 1, 1)
	local paths = self.paths()

	selected_path.changed:add(function(path_vm)
		if path_vm then
			current_colour(path_vm.colour())
		end
	end)
	current_colour.changed:add(function(r, g, b)
		local path_vm = selected_path()
		if path_vm then
			path_vm.colour(r, g, b)
		end
	end)

	paths.inserted:add(function(i, path_vm)
		path_vm.selected = Computed(selected_path,
			function(selected) return path_vm == selected end)

		function path_vm:select()
			selected_path(path_vm)
		end
	end)
	paths.removed:add(function(i, path_vm)
		if path_vm == selected_path() then
			selected_path(nil)
		end
	end)
	paths.cleared:add(function() selected_path(nil) end)

	self.selected_path = selected_path
	self.current_colour = current_colour

	local base_add_path = self.add_path
	function self:add_path()
		local path_vm = base_add_path()
		path_vm.colour(current_colour())

		return path_vm
	end

	local base_remove_path = self.remove_path
	function self:remove_path(path_vm)
		path_vm = path_vm or selected_path()
		if not path_vm then
			return
		end

		if selected_path() == path_vm then
			selected_path(nil)
		end

		base_remove_path(self, path_vm)
	end

	return self
end
