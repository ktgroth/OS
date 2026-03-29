#ifndef __CPU_ISR
#define __CPU_ISR

#include "../../../types.h"

typedef struct __attribute__((packed)) {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags;
} interrupt_frame_t;

typedef void (*isr_handler_t)(interrupt_frame_t *frame);

void init_isr(void);
void isr_register_handler(uint8_t vector, isr_handler_t handler);
void isr_dispatch(interrupt_frame_t *frame);

extern void *isr_stub_table[48];

#endif

