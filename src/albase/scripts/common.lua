-- common.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

function clamp(value, min, max)
	return math.min(math.max(min, value), max)
end

function Observable(...)
	local value = {...}
	local watchers = {}

	return setmetatable(
		{
			watch = function(callback)
				table.insert(watchers, callback)
			end
		},
		{
			__call = function(_, ...)
				if select('#', ...) == 0 then
					return table.unpack(value)
				else
					value = {...}
					for _,callback in ipairs(watchers) do
						callback(...)
					end
				end
			end
		})
end

function Binding(observable, callback)
	local updating_self = false

	observable.watch(function(...)
		if not updating_self then
			callback(...)
		end
	end)

	callback(observable())

	return function(...)
		if select('#', ...) == 0 then
			return observable()
		else
			updating_self = true
			observable(...)
			updating_self = false
		end
	end
end

function ObservableArray(...)
	local values = {...}
	local length = select('#', ...)
	local insert_watchers = {}
	local remove_watchers = {}
	local update_watchers = {}
	local clear_watchers = {}

	local function check_index(i, min, max)
		if math.floor(i) ~= i then
			error('index must be an integer')
		elseif i < min or i > max then
			error('index out of bounds')
		end
	end

	local function build_iterator()
		return function(_, i)
			i = i + 1
			if i > length then
				return nil
			else
				return i, values[i]
			end
		end, nil, 0
	end

	return setmetatable(
		{
			watch_insert = function(callback)
				table.insert(insert_watchers, callback)
			end,
			watch_remove = function(callback)
				table.insert(remove_watchers, callback)
			end,
			watch_update = function(callback)
				table.insert(update_watchers, callback)
			end,
			watch_clear = function(callback)
				table.insert(clear_watchers, callback)
			end,

			insert = function(i_or_value, ...)
				local i, value
				if select('#', ...) == 0 then
					i = length + 1
					value = i_or_value
				else
					i = i_or_value
					value = select(1, ...)
					check_index(i, 1, length + 1)
				end

				length = length + 1
				table.insert(values, i, value)

				for _,callback in ipairs(insert_watchers) do
					callback(i, value)
				end
			end,
			remove = function(i)
				if i == nil then
					i = length
				end

				check_index(i, 1, length)

				local value = values[i]

				length = length - 1
				table.remove(values, i)

				for _,callback in ipairs(remove_watchers) do
					callback(i, value)
				end
			end,
			clear = function()
				values = {}
				length = 0

				for _,callback in ipairs(clear_watchers) do
					callback()
				end
			end
		},
		{
			__index = function(_, i) return values[i] end,
			__newindex = function(_, i, value)
				check_index(i, 1, length)

				values[i] = value

				for _,callback in ipairs(update_watchers) do
					callback(i, value)
				end
			end,
			__len = function(_)
				return length
			end,
			__ipairs = build_iterator,
			__pairs = build_iterator
		})
end
