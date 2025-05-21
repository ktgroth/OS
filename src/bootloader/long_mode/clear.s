[bits 64]

clear_long:
    push rdi
    push rax
    push rcx

    shl rdi, 0x08
    mov rax, rdi

    mov al, SPACE

    mov rdi, VGA_START
    mov rcx, VGA_EXTENT / 0x02

    rep stosw

    pop rcx
    pop rax
    pop rdi
    ret

SPACE equ ` `
