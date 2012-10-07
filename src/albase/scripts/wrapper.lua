-- wrapper.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

local _cache = setmetatable({}, {__mode = 'v'})

function wrap(ptr, class)
	if ptr == nil then
		return nil
	end

	if _cache[ptr] == nil then
		_cache[ptr] = class(ptr)
	end

	return _cache[ptr]
end

function make_free(command)
	return function(self)
		command(self._ptr)
		_cache[self._ptr] = nil
		self._ptr = nil
	end
end

function wrap_command(command)
	return function(self, ...) return command(self._ptr, ...) end
end

function make_accessor(getter, setter)
	return function(self, ...)
		if select('#', ...) == 0 then
			return getter(self._ptr)
		else
			return setter(self._ptr, ...)
		end
	end
end

function make_wrapped_accessor(getter, setter, class)
	return function(self,  ...)
		if select('#') == 0 then
			return wrap(getter(self._ptr), class)
		else
			return wrap(setter(self._ptr, ...), class)
		end
	end
end

function make_var_accessor(var)
	return make_accessor(commands.getter(var), commands.setter(var))
end
