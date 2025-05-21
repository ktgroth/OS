[bits 32]

elevate_protected:
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    lgdt [gdt_64_descriptor]
    jmp KCODE_SEG_64:init_lm

[bits 64]
init_lm:
    cli

    mov ax, KDATA_SEG_64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ss, ax

    jmp begin_long_mode
