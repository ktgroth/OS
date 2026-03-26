[bits 32]

%macro SET_ENTRY64 2
    mov dword[%1], %2
    mov dword[%1 + 4], 0x00
%endmacro

%macro SET_PTR 2
    mov eax, %2
    or eax, 0x00000003
    SET_ENTRY64 %1, eax
%endmacro

init_page_directory:
    mov esi, PAGE_LEVEL_4
    mov eax, ((1 + 1 + 4 + 2048) * 0x1000) / 8
.zero_region:
    SET_ENTRY64 esi, 0x00

    add esi, 0x08
    dec eax
    test eax, eax
    jnz .zero_region

    SET_PTR PAGE_LEVEL_4, PAGE_LEVEL_3

    SET_PTR PAGE_LEVEL_3 + 0x00, PAGE_LEVEL_2_0
    SET_PTR PAGE_LEVEL_3 + 0x08, PAGE_LEVEL_2_1
    SET_PTR PAGE_LEVEL_3 + 0x10, PAGE_LEVEL_2_2
    SET_PTR PAGE_LEVEL_3 + 0x18, PAGE_LEVEL_2_3


    xor ebx, ebx
.pdpt_loop:
    mov esi, PAGE_LEVEL_2_0
    mov eax, ebx
    imul eax, 0x1000
    add esi, eax

    xor ecx, ecx
.pdt_loop:
    mov eax, ebx
    imul eax, 512
    add eax, ecx
    imul eax, 0x1000
    add eax, PAGE_LEVEL_1_BASE

    mov edx, eax
    or edx, 0x00000003
    SET_ENTRY64 (esi + ecx*8), edx
;    mov [esi + ecx*8], edx

    mov edi, eax
    xor edx, edx
.pt_loop:
    mov eax, ebx
    imul eax, 512
    add eax, ecx
    imul eax, 512
    add eax, edx
    shl eax, 12

    or eax, 0x00000003
    SET_ENTRY64 (edi + edx*8), eax
 ;   mov [edi + edx*8], eax

    inc edx
    cmp edx, 512
    jl .pt_loop

    inc ecx
    cmp ecx, 512
    jl .pdt_loop

    inc ebx
    cmp ebx, 4
    jl .pdpt_loop

    mov eax, PAGE_LEVEL_4
    mov cr3, eax

    mov eax, cr4
    or eax, 0x00000020
    mov cr4, eax

    ret

