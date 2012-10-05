-- common.lua
-- Copyright (c) 2011-2012 James Deery
-- Released under the MIT license <http://opensource.org/licenses/MIT>.
-- See COPYING for details.

function clamp(value, min, max)
	return math.min(math.max(min, value), max)
end
