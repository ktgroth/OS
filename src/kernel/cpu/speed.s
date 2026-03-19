[bits 64]
default rel

section .text
global get_cpu_hz
extern tick

; uint64_t get_cpu_hz(uint32_t freq, uint32_t sample_ticks)
; args:
;   edi = freq
;   esi = sample_ticks
; returns:
;   rax = CPU frequency in Hz, or 0 on failure/timeout

get_cpu_hz:
    push rbx
    push r12
    push r13

    test edi, edi
    jz .fail1
    test esi, esi
    jz .fail2

    mov eax, 0x01
    xor ecx, ecx
    cpuid
    bt edx, 0x04
    jnc .fail3

    mov r12, qword [rel tick]
.wait_edge:
    hlt
    cmp qword [rel tick], r12
    je .wait_edge
 
    mov r12, qword [rel tick]

    xor eax, eax
    cpuid
    rdtsc
    shl rdx, 0x20
    or rax, rdx
    mov r13, rax

.edge_seen:
    hlt
    mov rax, qword [rel tick]
    sub rax, r12
    cmp rax, rsi
    jb .edge_seen

    xor eax, eax
    cpuid
    rdtsc
    shl rdx, 0x20
    or rax, rdx

    sub rax, r13

    mov ecx, edi
    mul rcx

    mov ecx, esi
    div rcx

.done:
    pop r13
    pop r12
    pop rbx
    ret

.fail1:
    mov eax, 1
    jmp .done

.fail2:
    mov eax, 2
    jmp .done

.fail3:
    mov eax, 3
    jmp .done

.fail4:
    mov eax, 4
    jmp .done

.fail5:
    mov eax, 5
    jmp .done

