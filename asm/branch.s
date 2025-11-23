b b1

b2:
mov x0, #0
add x0, x0, #1
b exit

b1:
reg
rpc

b b2
exit:
