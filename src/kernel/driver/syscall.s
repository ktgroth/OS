[bits 64]
default rel

section .text
global syscall_entry
extern syscall_dispatch
extern g_kernel_rsp0

%define USER_CS 0x1B
%define USER_SS 0x23

syscall_entry:
    cld

    mov r10, rsp
    mov rsp, [g_kernel_rsp0]
    and rsp, -16

    push r10
    push r11
    push rcx
    push rax
    push rdi
    push rsi
    push rdx

    sub rsp, 8
    lea rdi, [rsp + 8]
    call syscall_dispatch
    add rsp, 8

    mov rcx, [rsp + 32]
    mov r11, [rsp + 40]
    mov r10, [rsp + 48]

    add rsp, 56

    push qword USER_SS
    push r10
    push r11
    push qword USER_CS
    push rcx
    iretq

