[bits 16]

KCODE_SEG equ gdt_32_kcode - gdt_32_start
KDATA_SEG equ gdt_32_kdata - gdt_32_start
UCODE_SEG equ gdt_32_ucode - gdt_32_start
UDATA_SEG equ gdt_32_udata - gdt_32_start

gdt_32_start:
    gdt_32_null:
        dd 0x00000000
        dd 0x00000000

    gdt_32_kcode:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10011010   ; Present (1) Must be 1 for a valid segment
                        ; 0 (2)
                        ; Code Segment (1) Set to declear either CODE or DATA segment
                        ; Executable (1) Set to declare CODE segment
                        ; 0 (1)
                        ; Readable/Writable (1) For CODE segment this is readable
                        ; 0 (1)

        db 0b11001111   ; Granularity indicates the size of the limit value is scaled by
                            ; (The limit is in 4KiB blocks)
                        ; Size flag defines a 32-bit protected mode segment
                        ; 0
                        ; 0
        db 0x00

    gdt_32_kdata:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b11001111
        db 0x00

    gdt_32_ucode:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11111010
        db 0b11001111
        db 0x00

    gdt_32_udata:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0b11110010
        db 0b11001111
        db 0x00

    TSS_32:
        dw 0x0067
        dw 0x0000
        db 0x00
        db 0x89
        db 0x40
        db 0x00
gdt_32_end:

gdt_32_descriptor:
    dw gdt_32_end - gdt_32_start - 1
    dd gdt_32_start
