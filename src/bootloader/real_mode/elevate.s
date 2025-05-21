[bits 16]

elevate_bios:
    cli

    lgdt[gdt_32_descriptor]

    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp KCODE_SEG:init_pm

[bits 32]
init_pm:
    mov ax, KDATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    jmp KCODE_SEG:begin_protected
