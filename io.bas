function printStr(str, chn)
 asm
  lref 1
  trap 2
 end asm
end function

function printInt(chn, n)
 asm
  lref 1
  trap 3
 end asm
end function

function printTab(chn)
 asm
  trap 4
 end asm
end function

function printNL(chn)
 asm
  trap 5
 end asm
end function
