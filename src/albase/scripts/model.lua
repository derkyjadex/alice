-- model.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ModelPath = class(commands.model_path_new, function(self) end)
commands.model_path_register_ctor(ModelPath)

ModelPath.prototype.colour = make_var_accessor('model_path_colour')

function ModelPath.prototype.points(self)
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

Model = class(commands.model_shape_new, function(self) end)
commands.model_shape_register_ctor(Model)

Model.prototype.load = commands.model_shape_load
Model.prototype.save = commands.model_shape_save

function Model.prototype.paths(self)
	return {commands.model_shape_get_paths(self)}
end
Model.prototype.add_path = commands.model_shape_add_path
Model.prototype.remove_path = commands.model_shape_remove_path
