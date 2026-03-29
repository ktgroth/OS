#ifndef __CPU_IDT
#define __CPU_IDT

#include "../../../types.h"

typedef struct __attribute__((packed)) {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idtr_t;

void init_idt(void);
void idt_set_gate(uint8_t vector, void *handler, uint8_t flags, uint8_t ist);
void idt_load(const idtr_t *idtr);

#endif

