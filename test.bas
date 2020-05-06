include "io.bas"

a = 1

function foo(x)
 return bar(x) + bar(a)
end function

function bar(y)
 return y + 1
end function

function baz(q, r)
  return q * 10 + r
end function

b = 2

print "Hello, world!"
print foo(12)
print baz(4, 6)
