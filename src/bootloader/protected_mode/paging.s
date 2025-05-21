[bits 32]

init_page_directory:
    mov edi, PAGE_LEVEL_3
    or edi, 0x00000003
    mov [PAGE_LEVEL_4], edi

    mov edi, PAGE_LEVEL_2
    or edi, 0x00000003
    mov [PAGE_LEVEL_3], edi

    mov ecx, 0x00
.loop:
    mov eax, 0x00200000
    mul ecx
    or eax, 0x00000083
    mov [PAGE_LEVEL_2 + ecx * 0x08], eax

    inc ecx
    cmp ecx, 0x200
    jl .loop

    mov eax, PAGE_LEVEL_4
    mov cr3, eax

    mov eax, cr4
    or eax, 0x00000020
    mov cr4, eax

    ret
