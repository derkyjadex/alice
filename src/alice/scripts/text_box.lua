-- text_box.lua
-- Copyright (c) 2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

TextBox = Widget:derive(function(self, callback)
	Widget.init(self)

	local text_size = 24
	local char_width, char_height = text_size * 100 / 256, text_size
	local padding = 3

	self:border_width(2)
	self:text_size(text_size)
	self:text_location(padding, padding)
	self:fill_colour(0, 0, 0, 1)

	local cursor = Widget()
	cursor:bounds(0, 0, 2, 0)
	cursor:fill_colour(1, 1, 1, 1)
	self:add_child(cursor)

	local value = ''
	local value_length = 0
	local cursor_pos = 0

	local function update()
		self:text(value)
		cursor:location(cursor_pos * char_width + padding, padding)
		cursor:invalidate()
	end

	update()

	function self.insert_text(_, text)
		local inserted_length
		value, inserted_length = commands.text_insert(value, text, cursor_pos + 1)
		value_length = value_length + inserted_length
		cursor_pos = cursor_pos + inserted_length
		update()
	end

	function self.back_delete()
		if cursor_pos > 0 then
			value = commands.text_remove(value, cursor_pos, 1)
			value_length = value_length - 1
			cursor_pos = cursor_pos - 1
			update()
		end
	end

	function self.forward_delete()
		if cursor_pos < value_length then
			value = commands.text_remove(value, cursor_pos + 1, 1)
			value_length = value_length - 1
			update()
		end
	end

	self:bind_down(function()
		self:grab_keyboard()
	end)

	self:bind_keyboard_lost(function()
		cursor:bounds(0, 0, 2, 0)
		cursor:invalidate()
	end)

	self:bind_key(function(_, key)
		if key == 276 then
			cursor_pos = clamp(cursor_pos - 1, 0, value_length)
			update()

		elseif key == 275 then
			cursor_pos = clamp(cursor_pos + 1, 0, value_length)
			update()

		elseif key == 8 then
			self:back_delete()

		elseif key == 127 then
			self:forward_delete()
		end
	end)

	self:bind_text(self.insert_text)

	function self.grab_keyboard(self)
		Widget.prototype.grab_keyboard(self)
		cursor:bounds(0, 0, 2, char_height)
		cursor:invalidate()
	end

	function self.layout(self, left, width, right, bottom, height, top)
		return Widget.prototype.layout(self, left, width, right, bottom, text_size + padding * 2, top)
	end
end)
