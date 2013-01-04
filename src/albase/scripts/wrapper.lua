-- wrapper.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

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

function wrap_ctor(type, init)
	local wrapper = commands[type .. '_wrap_ctor']
	init = init or function() end

	return wrapper(function(ctor)
		return class(ctor, init)
	end)
end
