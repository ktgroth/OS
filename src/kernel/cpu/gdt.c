
#include "../include/cpu/gdt.h"

uint64_t gdt_runtime[] = {
    0x0000000000000000ULL, 
    0x00AF9A000000FFFFULL,
    0x00AF92000000FFFFULL,
    0x00AFFA000000FFFFULL,
    0x00AFF2000000FFFFULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL
};

gdtr_t gdt_runtime_ptr = {
    .limit = (uint16_t)(sizeof(gdt_runtime) - 1),
    .base = (uint64_t)&gdt_runtime[0]
};

void gdt_runtime_load(void) {
    __asm__ __volatile__("lgdt %0" : : "m"(gdt_runtime_ptr));

    __asm__ __volatile__(
        "movw %0, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        :
        : "i"(GDT_SEL_KDATA)
        : "ax", "memory"
    );

    __asm__ __volatile__(
        "pushq %0\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : "i"(GDT_SEL_KCODE)
        : "rax", "memory"
    );
}

