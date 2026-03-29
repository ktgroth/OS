
#include "../include/cpu/idt.h"
#include "../include/cpu/gdt.h"

#define IDT_ENTRIES 256


static idt_entry_t g_idt[IDT_ENTRIES];
static idtr_t g_idtr;


void idt_set_gate(uint8_t vector, void *handler, uint8_t flags, uint8_t ist) {
    uint64_t addr = (uint64_t)handler;
    idt_entry_t *e = &g_idt[vector];

    e->offset_lo = (uint16_t)(addr & 0xFFFF);
    e->selector = get_kcode();
    e->ist = (uint8_t)(ist & 0x7);
    e->type_attr = flags;
    e->offset_mid = (uint16_t)((addr >> 16) & 0xFFFF);
    e->offset_hi = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    e->zero = 0;
}

void init_idt(void) {
    uint32_t i;
    for (i = 0; i < IDT_ENTRIES; ++i) {
        g_idt[i].offset_lo = 0;
        g_idt[i].selector = 0;
        g_idt[i].ist = 0;
        g_idt[i].type_attr = 0;
        g_idt[i].offset_mid = 0;
        g_idt[i].offset_hi = 0;
        g_idt[i].zero = 0;
    }

    g_idtr.limit = (uint16_t)(sizeof(g_idt) - 1);
    g_idtr.base = (uint64_t)g_idt;
    idt_load(&g_idtr);
}
