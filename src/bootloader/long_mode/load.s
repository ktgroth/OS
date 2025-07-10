
[bits 64]

ata_read:
    pushfq
    and rax, 0x0FFFFFFF
    push rax
    push rbx
    push rcx
    push rdx

    mov rbx, rax

    mov edx, 0x01F6
    shr eax, 0x18
    or al, 0b11100000
    out dx, al

    mov edx, 0x01F2
    mov al, cl
    out dx, al

    mov edx, 0x01F3
    mov eax, ebx
    out dx, al

    mov edx, 0x01F4
    mov eax, ebx
    shr eax, 0x08
    out dx, al

    mov edx, 0x01F5
    mov eax, ebx
    shr eax, 0x10
    out dx, al

    mov edx, 0x01F7
    mov al, 0x20
    out dx, al

    mov bl, cl
.read_sectors:

    mov dx, 0x01F7
.wait_bsy:
    in al, dx
    test al, 0x80
    jnz .wait_bsy

.wait_drq:
    in al, dx
    test al, 0x08
    jz .wait_drq

    mov rcx, 0x100
    mov rdx, 0x01F0
    rep insw

    sub bl, 0x01
    test bl, bl
    jnz .read_sectors

    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq

    ret
