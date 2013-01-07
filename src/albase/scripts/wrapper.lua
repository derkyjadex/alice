-- wrapper.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local _ctors = setmetatable({}, {__mode = 'k'})

local function derive(base, init)
	init = init or base.init
	local prototype = {}
	prototype.__index = prototype
	setmetatable(prototype, base.prototype)

	local ctor = _ctors[base]
	local class = setmetatable(
		{
			init = init,
			prototype = prototype,
			derive = derive
		},
		{
			__call = function(self, ...)
				local obj = ctor(prototype)

				init(obj, ...)

				return obj
			end
		})

	_ctors[class] = ctor

	return class
end

function wrap(type_name, init)
	local wrap_ctor = commands[type_name .. '_wrap_ctor']
	local set_prototype = commands[type_name .. '_set_prototype']

	init = init or function() end
	local prototype = {}
	prototype.__index = prototype

	local base_ctor
	local class = wrap_ctor(function(ctor)
		base_ctor = function(prototype)
			return set_prototype(ctor(), prototype)
		end

		return setmetatable(
			{
				init = init,
				prototype = prototype,
				derive = derive
			},
			{
				__call = function(self, ...)
					local obj = base_ctor(prototype)

					init(obj, ...)

					return obj
				end
			})
	end)

	_ctors[class] = base_ctor

	return class
end

function make_accessor(getter, setter)
	return function(self, ...)
		if select('#', ...) == 0 then
			return getter(self)
		else
			return setter(self, ...)
		end
	end
end

function make_var_accessor(var)
	return make_accessor(commands.getter(var), commands.setter(var))
end
