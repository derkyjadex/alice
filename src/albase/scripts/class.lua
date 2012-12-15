-- class.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local factories = setmetatable({}, {__mode = 'k'})
local mts = setmetatable({}, {__mode = 'k'})

local function _class(base, factory, init)
	local class = {}
	local prototype = {}
	local mt = {}

	if not base then
		factory = factory or function() return {} end
		init = init or function() end

		mt.__index = function(self, k)
			return prototype[k]
		end

	else
		factory = factories[base]
		init = init or base.init

		local base_mt_index = mts[base].__index

		mt.__index = function(self, k)
			local v = prototype[k]
			if v ~= nil then
				return v
			else
				return base_mt_index(self, k)
			end
		end
	end

	class.base = base
	class.init = init
	class.prototype = prototype
	factories[class] = factory
	mts[class] = mt

	return setmetatable(class, {
		__call = function(self, ...)
			local obj = setmetatable(factory(), mt)

			init(obj, ...)

			return obj
		end
	})
end

function class(base_or_factory_or_init, init)
	local base, factory

	if type(base_or_factory_or_init) == 'function' then
		if init then
			factory = base_or_factory_or_init
		else
			init = base_or_factory_or_init
		end

	elseif type(base_or_factory_or_init) == 'table' then
		base = base_or_factory_or_init
	end

	return _class(base, factory, init)
end
