-- common.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

function clamp(value, min, max)
	return math.min(math.max(min, value), max)
end

function Multicast()
	return setmetatable(
		{},
		{
			__call = function(self, ...)
				for _, f in ipairs(self) do
					f(...)
				end
			end,
			__index = {
				add = function(self, f)
					table.insert(self, f)
					return f
				end,
				remove = function(self, f)
					for i, current in ipairs(self) do
						if current == f then
							table.remove(self, i)
							return
						end
					end
				end
			},
			__newindex = function()
				error('cannot change properties of Multicast')
			end
		})
end

function Observable(...)
	local value = {...}
	local changed = Multicast()

	return setmetatable(
		{},
		{
			__call = function(_, ...)
				if select('#', ...) == 0 then
					return table.unpack(value)
				else
					value = {...}
					changed(...)
				end
			end,
			__index = {
				changed = changed
			},
			__newindex = function()
				error('cannot change properties of Observable')
			end
		})
end

function Binding(observable, callback)
	local updating_self = false

	local handle = observable.changed:add(function(...)
		if not updating_self then
			callback(...)
		end
	end)

	callback(observable())

	return setmetatable(
		{},
		{
			__call = function(_, ...)
				if select('#', ...) == 0 then
					return observable()
				else
					updating_self = true
					observable(...)
					updating_self = false
				end
			end,
			__index = {
				unbind = function()
					observable.changed:remove(handle)
				end
			},
			__newindex = function()
				error('cannot change properties of Binding')
			end
		})
end

function ObservableArray(...)
	local values = {...}
	local length = select('#', ...)
	local inserted = Multicast()
	local removed = Multicast()
	local updated = Multicast()
	local cleared = Multicast()

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

	local properties = {
		inserted = inserted,
		removed = removed,
		updated = updated,
		cleared = cleared,

		insert = function(_, i_or_value, ...)
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
			inserted(i, value)
		end,
		remove = function(_, i)
			i = i or length
			check_index(i, 1, length)

			local value = values[i]

			length = length - 1
			table.remove(values, i)
			removed(i, value)
		end,
		clear = function()
			length = 0
			values = {}
			cleared()
		end
	}

	return setmetatable(
		{},
		{
			__index = function(_, i)
				if type(i) == 'number' then
					return values[i]
				else
					return properties[i]
				end
			end,
			__newindex = function(_, i, value)
				if type(i) == 'number' then
					check_index(i, 1, length)
					values[i] = value
					updated(i, value)
				else
					error('cannot change properties of ObservableArray')
				end
			end,
			__len = function(_) return length end,
			__ipairs = build_iterator,
			__pairs = build_iterator
		})
end

function Computed(source, f)
	local value = {f(source())}
	local changed = Multicast()

	source.changed:add(function(...)
		value = {f(...)}
		changed(table.unpack(value))
	end)

	return setmetatable(
		{},
		{
			__call = function(_, ...)
				if select('#', ...) == 0 then
					return table.unpack(value)
				else
					error('cannot set value of computed')
				end
			end,
			__index = {
				changed = changed
			},
			__newindex = function()
				error('cannot change properties of Computed')
			end
		})
end
