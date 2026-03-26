[bits 16]

FB_INFO_BASE    equ 0x7000
VBE_CTRL_INFO   equ 0x7200
VBE_MODE_INFO   equ 0x7400

init_vbe:
    push ax
    push bx
    push cx
    push dx
    push di
    push es

    xor ax, ax
    mov es, ax

    mov byte [FB_INFO_BASE + 0], 0x00

    mov dword [VBE_CTRL_INFO], 0x32455256
    mov ax, 0x4F00
    mov di, VBE_CTRL_INFO
    int 0x10
    cmp ax, 0x004F
    jne .done

    mov ax, 0x4F01
    mov cx, 0x0118
    mov di, VBE_MODE_INFO
    int 0x10
    cmp ax, 0x004F
    jne .done

    mov ax, 0x4F02
    mov bx, 0x4118
    int 0x10
    cmp ax, 0x004F
    jne .done

    mov ax, [VBE_MODE_INFO + 0x10]
    mov [FB_INFO_BASE + 4], ax
    xor dx, dx
    mov [FB_INFO_BASE + 6], dx

    mov ax, [VBE_MODE_INFO + 0x12]
    mov [FB_INFO_BASE + 8], ax
    xor dx, dx
    mov [FB_INFO_BASE + 10], dx

    mov ax, [VBE_MODE_INFO + 0x14]
    mov [FB_INFO_BASE + 12], ax
    xor dx, dx
    mov [FB_INFO_BASE + 14], dx

    mov al, [VBE_MODE_INFO + 0x19]
    mov [FB_INFO_BASE + 1], al

    mov eax, [VBE_MODE_INFO + 0x28]
    mov [FB_INFO_BASE + 16], eax
    xor eax, eax
    mov [FB_INFO_BASE + 20], eax

    mov byte [FB_INFO_BASE + 0], 1

.done:
    pop es
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

