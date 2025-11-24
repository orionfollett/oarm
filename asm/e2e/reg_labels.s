.reg i, x0
.reg max, x1

mov i, #0
mov max #100

loop:
str i, [i]
add i, i, #1
cmp i, max
blt loop
mem
ret