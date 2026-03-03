
#include "../include/cpu/idt.h"

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

void set_idt_gate_with_flags(int32_t gate_num, uint64_t handler, uint8_t flags) {
    uint16_t low_16 = (uint16_t) (handler & 0xFFFF);
    uint16_t middle_16 = (uint16_t) ((handler >> 16) & 0xFFFF);
    uint32_t high_16 = (uint32_t) ((handler >> 32) & 0xFFFFFFFF);
    
    idt_gate_t gate = {
        .low_offset = low_16,
        .sel = KERNEL_CS,
        .always0 = 0,
        .flags = flags,
        .middle_offset = middle_16,
        .high_offset = high_16,
        .reserved = 0
    };

    idt[gate_num] = gate;
}

void set_idt_gate(int32_t gate_num, uint64_t handler) {
    set_idt_gate_with_flags(gate_num, handler, 0x8E);
}

void set_idt(void) {
    idt_reg.base = (uint64_t)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;

    __asm__ __volatile__("lidt (%0)" : : "r" (&idt_reg));
}
