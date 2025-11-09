.global _asm_sort

start .req x0
len .req x1
last_byte .req x2
min_index .req x3
min .req x4
i .req x5
j .req x6
curr_i .req x7
curr_j .req x8
temp .req x9

_asm_sort:
    
    // convert len to bytes, multiply by 4
    lsl len, len, #3
    add last_byte, len, start
    
    mov i, start
    outer:
        cmp last_byte, i
        blt exit

        ldr curr_i, [i]
        mov min_index, i
        mov min, curr_i
        
        add j, i, #8

        inner:    
            cmp j, last_byte
            bgt inner_exit

            ldr curr_j, [j]

            cmp curr_j, min
            bge not_min
                // found new min
                mov min, curr_j
                mov min_index, j
            not_min:
                add j, j, #8
        b inner
        inner_exit:

        // insert min into beginning of outer loop
        str min, [i]
        str curr_i, [min_index]
        add i, i, #8

        
        b outer
    
    exit:
    
    ret