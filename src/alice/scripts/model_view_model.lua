-- model_view_model.lua
-- Copyright (c) 2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local function smooth_point(point_vm)
	local x1, y1 = point_vm.prev.location()
	local x2, y2 = point_vm.next.location()

	local x = 0.5 * x1 + 0.5 * x2
	local y = 0.5 * y1 + 0.5 * y2

	point_vm.location(x, y)
end

local function ModelPointViewModel(path_vm, p, index)
	index = Observable(index)
	local location = Observable(p[1], p[2])

	local self
	self = {
		path = function() return path_vm end,
		index = index,
		location = location,
		remove = function() path_vm.remove(self) end
	}

	location.watch(function(x, y)
		path_vm.path():set_point(index(), x, y)

		smooth_point(self.prev)
		smooth_point(self.next)
	end)

	return self
end

local function ModelMidPointViewModel(path_vm, p, index)
	index = Observable(index)
	local location = Observable(p[1], p[2])

	local self
	self = {
		index = index,
		location = location,
		subdivide = function() path_vm.subdivide(self) end
	}

	location.watch(function(x, y)
		path_vm.path():set_point(index(), x, y)
	end)

	return self
end

local function ModelPathViewModel(model_vm, path, index)
	local point_vms = ObservableArray()
	local mid_point_vms = ObservableArray()

	local colour = Observable(path:colour())
	colour.watch(function(r, g, b)
		path:colour(r, g, b)
	end)

	local self
	self = {
		index = Observable(index),
		colour = colour,
		points = function() return point_vms end,
		mid_points = function() return mid_point_vms end,
		remove_point = function(point_vm)
			local p1, p2, p3 = point_vm.prev, point_vm, point_vm.next
			local i1, i2, i3 = p1.index(), p2.index(), p3.index()

			if i3 < i1 then
				path:remove(i2)
				path:remove(i1)

				i3 = i1
				p1, p3 = p3, p1

				p1.prev = p3.prev
				p3.prev.next = p1
			else
				path:remove(i3)
				path:remove(i2)

				p1.next = p3.next
				p3.next.prev = p1
			end

			mid_point_vms.remove((i3 + 1) / 2)
			for i = (i3 + 1) / 2, #mid_point_vms do
				mid_point_vms[i].index(i * 2 - 1)
			end

			point_vms.remove(i2 / 2)
			for i = i2 / 2, #point_vms do
				point_vms[i].index(i * 2)
			end

			smooth_point(p1)
		end,
		subdivide = function(mid_point_vm)
			local p1, p4 = mid_point_vm, mid_point_vm.next
			local i4 = p4.index()
			local x, y = p1.location()

			path:add_point(i4, x, y)
			path:add_point(i4, x, y)

			local i2, i3 = i4, i4 + 1
			local p2 = ModelPointViewModel(self, {x, y}, i2)
			local p3 = ModelMidPointViewModel(self, {x, y}, i3)

			p1.next = p2
			p2.prev, p2.next = p1, p3
			p3.prev, p3.next = p2, p4
			p4.prev = p3

			point_vms.insert(i2 / 2, p2)
			for i = i2 / 2, #point_vms do
				point_vms[i].index(i * 2)
			end

			mid_point_vms.insert((i3 + 1) / 2, p3)
			for i = (i3 + 1) / 2, #mid_point_vms do
				mid_point_vms[i].index(i * 2 - 1)
			end

			smooth_point(p1)
			smooth_point(p3)
		end,
		path = function() return path end
	}

	local function rebuild_point_vms()
		point_vms.clear()
		mid_point_vms.clear()

		for i, p in ipairs(path:points()) do
			local prev, point_vm

			if i % 2 == 0 then
				prev = mid_point_vms[#mid_point_vms]
				point_vm = ModelPointViewModel(self, p, i)
				point_vms.insert(point_vm)
			else
				prev = point_vms[#point_vms]
				point_vm = ModelMidPointViewModel(self, p, i)
				mid_point_vms.insert(point_vm)
			end

			if prev then
				prev.next = point_vm
				point_vm.prev = prev
			end
		end

		local prev = point_vms[#point_vms]
		prev.next = mid_point_vms[1]
		mid_point_vms[1].prev = prev
	end

	rebuild_point_vms()

	return self
end

function ModelViewModel(model)
	model = model or Model()
	local path_vms = ObservableArray()

	local self
	local function rebuild_path_vms()
		path_vms.clear()

		for i, path in ipairs(model:paths()) do
			path_vms.insert(ModelPathViewModel(self, path, i))
		end
	end

	self = {
		paths = function() return path_vms end,
		add_path = function()
			local path = model:add_path(0, 0, -20, 20, -20)
			path:add_point(0, 20, 0)
			path:add_point(0, 20, 20)
			path:add_point(0, 0, 20)
			path:add_point(0, -20, 20)
			path:add_point(0, -20, 0)
			path:add_point(0, -20, -20)

			local path_vm = ModelPathViewModel(self, path, #path_vms + 1)
			path_vms.insert(path_vm)

			return path_vm
		end,
		remove_path = function(path_vm)
			local index = path_vm.index()
			model:remove_path(index)
			path_vms.remove(index)

			for i = index, #path_vms do
				path_vms[i].index(i)
			end
		end,
		load = function(filename)
			model:load(filename)
			rebuild_path_vms()
		end,
		save = function(filename)
			model:save(filename)
		end,
		model = function() return model end
	}

	rebuild_path_vms()

	return self
end

function ModelContextViewModel(model)
	self = ModelViewModel(model)
	local selected_path = Observable()
	local current_colour = Observable(1, 1, 1)

	selected_path.watch(function(path_vm)
		if path_vm then
			current_colour(path_vm.colour())
		end
	end)
	current_colour.watch(function(r, g, b)
		local path_vm = selected_path()
		if path_vm then
			path_vm.colour(r, g, b)
		end
	end)

	self.paths().watch_insert(function(i, path_vm)
		path_vm.select = function()
			selected_path(path_vm)
		end
	end)
	self.paths().watch_remove(function(i, path_vm)
		if path_vm == selected_path() then
			selected_path(nil)
		end
	end)
	self.paths().watch_clear(function() selected_path(nil) end)

	self.selected_path = selected_path
	self.current_colour = current_colour

	local base_remove_path = self.remove_path
	self.remove_path = function(path_vm)
		path_vm = path_vm or selected_path()
		if not path_vm then
			return
		end

		if selected_path() == path_vm then
			selected_path(nil)
		end

		base_remove_path(path_vm)
	end

	return self
end
