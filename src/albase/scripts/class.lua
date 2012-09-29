-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

function class(base, init)
	local class = {}
	class.prototype = {}

	if not init and type(base) == 'function' then
		init = base
		base = nil

	elseif type(base) == 'table' then
		for i, v in pairs(base.prototype) do
			class.prototype[i] = v
		end
		class._base = base
	end

	class.init = init
	class.prototype.__index = class.prototype

	return setmetatable(class, {
		__call = function(self, ...)
			local obj = setmetatable({}, class.prototype)

			if init then
				init(obj, ...)
			elseif base and base.init then
				base.init(obj, ...)
			end

			return obj
		end
	})
end
