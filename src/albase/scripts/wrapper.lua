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

function make_accessor(command)
	return function(self, ...) return command(self._ptr, ...) end
end

function make_wrapped_accessor(command, class)
	return function(self, ...) return wrap(command(self._ptr, ...), class) end
end

function make_var_getter(var)
	return function(self) return commands.get(var, self._ptr) end
end

function make_var_setter(var)
	return function(self, ...) return commands.set(var, self._ptr, ...) end
end
