-- file_widget.lua
-- Copyright (c) 2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

FileWidget = Widget:derive(function(self, type)
	Widget.init(self)

	local can_save = type == 'save'

	local callback = function() end

	self:border_width(2)
	self:fill_colour(0.2, 0.3, 0.3, 1)

	local cancel_widget = Widget():add_to(self)
		:fill_colour(0.7, 0.5, 0.1, 1.0)
		:text('Cancel')
		:text_size(24)
		:text_location(3, 3)

	local save_widget, filename_widget
	if can_save then
		save_widget = Widget():add_to(self)
			:fill_colour(0.0, 0.6, 0.1, 1.0)
			:text('Save')
			:text_size(24)
			:text_location(3, 3)

		filename_widget = TextBox():add_to(self)
	end

	local path_widget = Widget():add_to(self)
		:fill_colour(0.15, 0.15, 0.15, 1.0)
		:border_width(2)
		:text_size(24)
		:text_location(3, 3)

	local list_widget = Widget():add_to(self)
		:fill_colour(0.15, 0.15, 0.15, 1.0)
		:border_width(2)

	local path, set_path, update_files, layout_files

	set_path = function(new_path)
		path = new_path
		path_widget:text(path):invalidate()
		update_files()
	end

	local file_widgets = {}

	update_files = function()
		for i, widget in ipairs(file_widgets) do
			widget:remove()
		end
		file_widgets = {}

		for i, file in ipairs{commands.fs_list_dir(path)} do
			local widget = Widget():add_to(list_widget)
				:fill_colour(0, 0, 0, 1)
				:text(file.name)
				:text_size(24)
				:text_location(3, 3)

			if file.is_dir then
				widget:text_colour(0.8, 0.7, 0.6)
					:bind_down(function()
						set_path(file.path)
					end)
			else
				widget:bind_down(function()
					callback(file.path)
				end)
			end

			table.insert(file_widgets, widget)
		end

		layout_files()
	end

	layout_files = function()
		local top = 5
		for i, widget in ipairs(file_widgets) do
			widget:layout(5, nil, 5, nil, 30, top)
			top = top + 35
		end
	end

	function self:bind_result(new_callback)
		callback = new_callback

		return self
	end

	function self:layout(left, width, right, bottom, height, top)
		Widget.prototype.layout(self, left, width, right, bottom, height, top)

		cancel_widget:layout(10, 70, nil, 10, 30, nil)

		if can_save then
			save_widget:layout(nil, 70, 10, 10, 30, nil)
			filename_widget:layout(10, nil, 10, 50, nil, nil)

			list_widget:layout(10, nil, 10, 90, nil, 50)
		else
			list_widget:layout(10, nil, 10, 50, nil, 50)
		end

		path_widget:layout(10, nil, 10, nil, 30, 10)

		layout_files()

		return self
	end

	cancel_widget:bind_down(function()
		callback(nil)
	end)

	if can_save then
		save_widget:bind_down(function()
			callback(commands.fs_path_append_filename(path, filename_widget:text()))
		end)

		filename_widget:grab_keyboard()
	end

	set_path(commands.fs_get_home_dir())
end)
