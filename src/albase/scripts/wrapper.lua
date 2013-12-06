-- wrapper.lua
-- Copyright (c) 2011-2013 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local vars = require 'vars'
local wrapper = require 'wrapper'

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
	init = init or function() end
	local prototype = {}
	prototype.__index = prototype

	local ctor
	local class = wrapper.wrap_ctor(type_name, function(base_ctor)
		ctor = function(prototype)
			local obj = base_ctor()
			return wrapper.set_prototype(obj, prototype)
		end

		return setmetatable(
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
	end)

	_ctors[class] = ctor

	return class
end

function make_accessor(getter, setter)
	return function(self, ...)
		if select('#', ...) == 0 then
			return getter(self)
		else
			setter(self, ...)
			return self
		end
	end
end

function make_var_accessor(var)
	return make_accessor(vars.getter(var), vars.setter(var))
end
