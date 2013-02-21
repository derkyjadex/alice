-- model_view_model.lua
-- Copyright (c) 2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

function ModelPointViewModel()
	return {
		location = Observable(0, 0)
		selected = Observable(false)
	}
end

function ModelMidPointViewModel()
	return {
		location = Observable(0, 0)
	}
end

function ModelPathViewModel(path)
	-- subdivide, move_point, remove_point

	return {
		colour = Observable(path:colour())
		path = function() return path end
	}
end

function ModelViewModel(model)
	model = model or Model()

	-- add_path, remove_path
	-- current_path, current_point

	return {
		model = function() return model end
	}
end
