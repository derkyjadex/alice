-- model.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ModelPath = class(function(self, ptr, model)
	commands.model_path_register(self, ptr)

	function self.model()
		return model
	end
end)
commands.model_path_register_ctor(ModelPath)

ModelPath.prototype.colour = make_accessor(
	commands.model_path_get_colour,
	commands.model_path_set_colour)

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

Model = class(function(self)
	commands.model_new(self)
end)

Model.prototype.load = commands.model_load
Model.prototype.save = commands.model_save

function Model.prototype.paths(self)
	return {commands.model_get_paths(self)}
end
Model.prototype.add_path = commands.model_add_path
Model.prototype.remove_path = commands.model_remove_path
