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
			__call = function(self, ...)
				if select('#', ...) == 0 then
					return self:get()
				else
					return self:set(...)
				end
			end,
			__index = {
				get = function()
					return table.unpack(value)
				end,
				set = function(self, ...)
					value = {...}
					changed(...)
				end,
				changed = changed
			},
			__newindex = function()
				error('cannot change properties of Observable')
			end
		})
end

function Property(...)
	local observable = Observable(...)

	return setmetatable(
		{},
		{
			__call = function(self, owner, ...)
				if select('#', ...) == 0 then
					return observable:get()
				else
					observable:set(...)
					return owner
				end
			end,
			__index = {
				get = function()
					return observable:get()
				end,
				set = function(self, ...)
					return observable:set(...)
				end,
				changed = observable.changed
			},
			__newindex = function()
				error('cannot change properties of Property')
			end
		})
end

function Binding(source, callback)
	local updating_self = false

	local handle = source.changed:add(function(...)
		if not updating_self then
			callback(...)
		end
	end)

	callback(source:get())

	return setmetatable(
		{},
		{
			__call = function(self, ...)
				if select('#', ...) == 0 then
					return self:get()
				else
					return self:set(...)
				end
			end,
			__index = {
				get = function()
					return source:get()
				end,
				set = function(self, ...)
					updating_self = true
					source:set(...)
					updating_self = false
				end,
				unbind = function()
					source.changed:remove(handle)
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
	local value = {f(source:get())}
	local changed = Multicast()

	source.changed:add(function(...)
		value = {f(...)}
		changed(table.unpack(value))
	end)

	return setmetatable(
		{},
		{
			__call = function(self, ...)
				if select('#', ...) == 0 then
					return self:get()
				else
					error('cannot set value of computed')
				end
			end,
			__index = {
				get = function()
					return table.unpack(value)
				end,
				changed = changed
			},
			__newindex = function()
				error('cannot change properties of Computed')
			end
		})
end
