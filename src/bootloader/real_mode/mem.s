[bits 16]

MAGIC_NUMBER equ 0x534D4150

detect_mem:
    cli

    push eax
    push ebx
    push ecx
    push edx
    push edi
    push es

    mov edi, E820_BUFFER
    push word di
    shr edi, 0x10
    mov es, di
    pop word di

    xor ebx, ebx
.next_entry:
    mov eax, 0xE820
    mov cx, 0x18
    mov edx, MAGIC_NUMBER

    int 0x15

    cmp eax, MAGIC_NUMBER
    jne .error

    add di, E820_ENTRY_SIZE

    cmp ebx, 0x00
    jne .next_entry

.done:
    pop es
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax

    sti
    ret

.error:
    hlt
