
#ifndef __CPU_IDT
#define __CPU_IDT

#include "../libc/types.h"

#define KERNEL_CS 0x08

typedef struct __attribute__((packed)) {
    uint16_t low_offset;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t middle_offset;
    uint32_t high_offset;
    uint32_t reserved;
} idt_gate_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_register_t;

#define IDT_ENTRIES 256

void set_idt_gate(int32_t n, uint64_t handler);
void set_idt(void);

#endif
