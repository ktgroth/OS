
KCODE_SEG_64 equ gdt_64_kcode - gdt_64_start
KDATA_SEG_64 equ gdt_64_kdata - gdt_64_start
UCODE_SEG_64 equ gdt_64_ucode - gdt_64_start
UDATA_SEG_64 equ gdt_64_udata - gdt_64_start

gdt_64_start:
    gdt_64_null:
        dd 0x00000000
        dd 0x00000000

    gdt_64_kcode:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10011010
        db 0b10101111
        db 0x00

    gdt_64_kdata:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b10100000
        db 0x00

    gdt_64_ucode:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11111010
        db 0b10101111
        db 0x00

    gdt_64_udata:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11110010
        db 0b11001111
        db 0x00

    TSS_64:
        dw 0x0067
        dw 0x0000
        db 0x00
        db 0x89
        db 0xA0
        db 0x00
gdt_64_end:

gdt_64_descriptor:
    dw gdt_64_end - gdt_64_start - 1
    dd gdt_64_start
