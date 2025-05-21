
[extern isr_handler]
[extern irq_handler]

REGISTER_SIZE equ 0x78
QUADWORD_SIZE equ 0x08

%macro PUSHALL 0
    push rdi
    push rsi
    push rdx
    push rcx
    push rax
    push r8
    push r9
    push r10
    push r11
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POPALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r11
    pop r10
    pop r9
    pop r8
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
%endmacro

%macro SAVE_REGS_AND_CALL_HANDLER 1
    PUSHALL

    mov rdx, rsp
    mov rdi, [rsp + REGISTER_SIZE]
    mov rsi, [rsp + REGISTER_SIZE + QUADWORD_SIZE]

    call %1

    POPALL
%endmacro

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli

    push qword 0
    push qword %1
    SAVE_REGS_AND_CALL_HANDLER isr_handler

    add rsp, 0x10
    sti

    iretq
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli

    push qword %1
    SAVE_REGS_AND_CALL_HANDLER isr_handler

    add rsp, 0x10
    sti

    iretq
%endmacro

%macro IRQ_CODE 2
global irq%1
irq%1:
    cli

    push qword %1
    push qword %2
    SAVE_REGS_AND_CALL_HANDLER irq_handler

    add rsp, 0x10
    sti

    iretq
%endmacro

ISR_NOERRCODE 0     ; Division Error
ISR_NOERRCODE 1     ; Debug Exception
ISR_NOERRCODE 2     ; NMI Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device not Available

ISR_NOERRCODE 8     ; Dobule Fault

ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment Not Present
ISR_ERRCODE   12    ; Stack Segment Fault
ISR_ERRCODE   13    ; General Protection
ISR_ERRCODE   14    ; Page Fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; FPU Floating Point Error
ISR_NOERRCODE 17    ; Alignment Check
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating Point Exception
ISR_NOERRCODE 20    ; Virtualization Exception
ISR_NOERRCODE 21    ; Control Protection
ISR_NOERRCODE 22    ; Unknown
ISR_NOERRCODE 23    ; Unknown
ISR_NOERRCODE 24    ; Unknown
ISR_NOERRCODE 25    ; Unknown
ISR_NOERRCODE 26    ; Unknown
ISR_NOERRCODE 27    ; Unknown
ISR_NOERRCODE 28    ; Unknown
ISR_NOERRCODE 29    ; Unknown
ISR_NOERRCODE 30    ; Unknown
ISR_NOERRCODE 31    ; Unknown

IRQ_CODE 0, 32
IRQ_CODE 1, 33
IRQ_CODE 2, 34
IRQ_CODE 3, 35
IRQ_CODE 4, 36
IRQ_CODE 5, 37
IRQ_CODE 6, 38
IRQ_CODE 7, 39
IRQ_CODE 8, 40
IRQ_CODE 9, 41
IRQ_CODE 10, 42
IRQ_CODE 11, 43
IRQ_CODE 12, 44
IRQ_CODE 13, 45
IRQ_CODE 14, 46
IRQ_CODE 15, 47
