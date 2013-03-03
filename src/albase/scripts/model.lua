-- model.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ModelPath = wrap('model_path')
ModelPath.prototype.colour = make_var_accessor('model_path_colour')

function ModelPath.prototype:points()
	local values = {commands.model_path_get_points(self)}
	local points = {}
	for i = 1, #values, 2 do
		local point = {values[i], values[i + 1]}
		table.insert(points, point)
	end

	return points
end

ModelPath.prototype.set_point = commands.model_path_set_point
ModelPath.prototype.add_point = commands.model_path_add_point
ModelPath.prototype.remove_point = commands.model_path_remove_point
ModelPath.prototype.hit_test = commands.model_path_hit_test

local function build_path(self, data)
	local path = self:add_path(0, data[1], data[2], data[3], data[4])

	if #data > 4 then
		for j = 6, #data, 2 do
			path:add_point(j / 2, data[j - 1], data[j])
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
Model.prototype.load = commands.model_shape_load
Model.prototype.save = commands.model_shape_save

function Model.prototype:paths()
	return {commands.model_shape_get_paths(self)}
end
Model.prototype.add_path = commands.model_shape_add_path
Model.prototype.remove_path = commands.model_shape_remove_path
