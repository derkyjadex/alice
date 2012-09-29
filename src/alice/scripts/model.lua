-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

ModelPath = class(function(self, ptr)
	self._ptr = ptr
end)

ModelPath.prototype.get_colour = make_accessor(commands.model_path_get_colour)
ModelPath.prototype.set_colour = make_accessor(commands.model_path_set_colour)

ModelPath.prototype.get_points = make_accessor(commands.model_path_get_points)
function ModelPath.prototype.get_points(self)
	local values = {commands.model_path_get_points(self._ptr)}
	local points = {}
	for i = 1, #values, 2 do
		local point = {values[i], values[i + 1]}
		table.insert(points, point)
	end

	return points
end

ModelPath.prototype.set_point = make_accessor(commands.model_path_set_point)
ModelPath.prototype.add_point = make_accessor(commands.model_path_add_point)
ModelPath.prototype.remove_point = make_accessor(commands.model_path_remove_point)

Model = class(function(self, ptr)
	if not ptr then
		ptr = commands.model_new()
	end
	self._ptr = ptr
end)

Model.prototype.free = make_free(commands.model_free)
Model.prototype.load = make_accessor(commands.model_load)
Model.prototype.save = make_accessor(commands.model_save)

function Model.prototype.get_paths(self)
	local ptrs = {commands.model_get_paths(self._ptr)}
	local paths = {}
	for i, p in ipairs(ptrs) do
		paths[i] = wrap(p, ModelPath)
	end

	return paths
end
Model.prototype.add_path = make_accessor(commands.model_add_path)
Model.prototype.remove_path = make_accessor(commands.model_remove_path)
