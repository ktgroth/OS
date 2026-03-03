
[bits 64]
[org 0x0]

start:
    lea rdi, [rel msg]
    mov rax, 0x01
    int 0x80

    mov rax, 0x02
    mov rbx, 0x00
    int 0x80
    ret


msg: db "HELLO WORLD", 0x0A, 0x00

