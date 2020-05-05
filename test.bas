a = 1

function printStr(str, chn)
 asm
  lref 1
  trap 2
 end asm
end function

function printInt(n, chn)
 asm
  lref 1
  trap 3
 end asm
end function

function printNL(chn)
 asm
  trap 5
 end asm
end function

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
print baz(4,6)
