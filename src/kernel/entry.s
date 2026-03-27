[bits 64]
default rel

global _start
extern kmain

_start:
    cli
    lea rsp, [stack_top]
    call kmain
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

