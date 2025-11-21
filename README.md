Orion's Subset of ARM Assembly

Goal:
- continue practicing programming with manual memory management
- work on parsing
- learn more about assembly and how computers work
- develop mastery in a low level language

Why C?
- timeless, lots of legacy code in C
- understanding C helps understand most every other language, as well as large codebases important to me like postgres and the linux kernel
- forces me to do everything from scratch, which is a good thing since this is all educational anyways

Project:

CPU emulator that supports a small subset of ARM assembly.

ARM supported:

- register labels
- mov
- ldr, str
- add, sub
- lsl, lsr
- cmp
- blt, ble, bgt, bge, b, beq, bne

My extensions for debugging:
rpc - prints program counter
reg - prints all registers
mem - prints all memory

How to run:
```
source dev.sh
run asm/all.s
```

Next project...
A compiler that generates my OARM asm from a simpler high level language? (use zig?)


TODO:
Branch design:

1. load all the asm (no interpreter/REPL anymore, only reading from file) (finally some dynamic memory!)

first pass through asm:
    - normalize whitespace # TODO
    - store all branch labels along with line num in a jump table

- pc initializes to 0
- increment pc on each tick
- each tick use the pc to decide which line to execute
- branch instructions modify the pc
- branch instructions reference labels, look up the label, change pc to eq that num in the jump table
