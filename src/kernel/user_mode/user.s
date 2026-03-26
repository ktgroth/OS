[bits 64]

section .text
global enter_user

%define USER_CS ((3 << 3) | 0x03)
%define USER_SS ((4 << 3) | 0x03)

; Assumes:
;   USER_CS = 0x1B
;   USER_SS = 0x23
;   rdi = user rip
;   rsi = user rsp
enter_user:
    cli
    mov ax, USER_SS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push USER_SS
    push rsi
    pushfq
    pop rax
    or rax, (1 << 9)
    push rax
    push USER_CS
    push rdi
    iretq

