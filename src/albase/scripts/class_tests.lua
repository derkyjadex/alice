require 'class'

function test_basic_prototype()
	local A = class()
	local a = A()
	A.prototype.f = function() return 5 end

	assert(a.f() == 5)

	a.f = function() return 6 end

	assert(a.f() == 6)

	a.f = nil

	assert(a.f() == 5)
end

function test_inherited_prototype()
	local A = class()
	local B = class(A)

	local a = A()
	local b = B()

	A.prototype.f = function() return 5 end

	assert(b.f() == 5)

	B.prototype.f = function() return 6 end

	assert(b.f() == 6)
	assert(a.f() == 5)
end

function test_basic_factory()
	local x = {}

	local A = class(
		function() return x end,
		function(self) end)

	local a = A()

	assert(a == x)
end

function test_inherited_factory()
	local x = {}

	local A = class(
		function() return x end,
		function(self) end)
	local B = class(A)

	local b = B()

	assert(b == x)
end

function test_basic_init()
	local inited, inited_param
	local x = {}

	local A = class(function(self, param)
		inited = self
		inited_param = param
	end)

	local a = A(x)

	assert(inited == a)
	assert(inited_param == x)
end

function test_inherited_default_init()
	local inited, inited_param
	local x = {}

	local A = class(function(self, param)
		inited = self
		inited_param = param
	end)
	local B = class(A)

	local b = B(x)

	assert(inited == b)
	assert(inited_param == x)
end

function test_inherited_overridden_init()
	local inited, inited_param
	local base_inited, base_inited_param
	local x = {}

	local A = class(function(self, param)
		base_inited = self
		base_inited_param = param
	end)
	local B = class(A, function(self, param)
		A.init(self, param)
		inited = self
		inited_param = param
	end)

	local b = B(x)

	assert(base_inited == b)
	assert(base_inited_param == x)

	assert(inited == b)
	assert(inited_param == x)
end

function test_set_class_property()
	local x = {}

	local A = class()
	A.property = x

	assert(A.property == x)
end


test_basic_prototype()
test_inherited_prototype()
test_basic_factory()
test_inherited_factory()
test_basic_init()
test_inherited_default_init()
test_inherited_overridden_init()
test_set_class_property()

print('All passed!')
