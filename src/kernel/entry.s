
[bits 64]
[extern main]

section .text
global _start
_start:
    mov rsp, 0x90000
    mov rbp, rsp
    call main

.hang:
    hlt
    jmp $
