-- model.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local model = require 'model'

ModelPath = wrap('model_path')
ModelPath.prototype.colour = make_var_accessor('model_path.colour')

function ModelPath.prototype:points()
	local values = {model.path_get_points(self)}
	local points = {}
	for i = 1, #values, 3 do
		local point = {values[i], values[i + 1], values[i + 2]}
		table.insert(points, point)
	end

	return points
end

ModelPath.prototype.set_point = model.path_set_point
ModelPath.prototype.add_point = model.path_add_point
ModelPath.prototype.remove_point = model.path_remove_point
ModelPath.prototype.hit_test = model.path_hit_test

local function build_path(self, data)
	local path = self:add_path(0, data[1], data[2], data[3], data[4], data[5], data[6])

	if #data > 6 then
		for j = 9, #data, 3 do
			path:add_point(j / 3, data[j - 2], data[j - 1], data[j])
		end
	end

	if data.colour then
		path:colour(data.colour[1], data.colour[2], data.colour[3])
	end
end

Model = wrap('model_shape', function(self, ...)
	for _,data in ipairs({...}) do
		build_path(self, data)
	end
end)
Model.prototype.load = model.shape_load
Model.prototype.save = model.shape_save

function Model.prototype:paths()
	return {model.shape_get_paths(self)}
end
Model.prototype.add_path = model.shape_add_path
Model.prototype.remove_path = model.shape_remove_path
