[bits 64]

print_long:
    push rax
    push rdx
    push rdi
    push rsi

    mov rdx, VGA_START
    shl rdi, 0x08

.loop:
    cmp byte [rsi], 0x00
    je .done

    cmp rdx, VGA_START + VGA_EXTENT
    je .done

    mov rax, rdi
    mov al, byte [rsi]

    mov word [rdx], ax

    add rsi, 0x01
    add rdx, 0x02

    jmp .loop

.done:
    pop rsi
    pop rdi
    pop rdx
    pop rcx

    ret
