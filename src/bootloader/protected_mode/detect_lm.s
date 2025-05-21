[bits 32]

detect_lm_protected:
    pushad

    pushfd
    pop eax

    mov ecx, eax
    xor eax, 0x01 << 0x15

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    cmp eax, ecx
    je .cpuid_not_found

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .cpuid_not_found

    mov eax, 0x80000001
    cpuid
    test edx, 0x01 << 0x1D
    jz .lm_not_found

    popad
    ret

.cpuid_not_found:
    hlt

.lm_not_found:
    hlt
