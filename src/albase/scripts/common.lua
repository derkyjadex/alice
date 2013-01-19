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
