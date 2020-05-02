100 a=1
110 function foo(x)
120  return bar(x) + bar(a)
130 end function
140 function bar(y)
150  return y+1
160 end function
170 b=2
180 foo(12)
