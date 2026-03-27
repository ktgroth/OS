
#include "../include/cpu/tss.h"
#include "../include/libc/stdlib.h"

static tss64_t g_tss;
uint64_t g_kernel_rsp0 = 0;

void tss_init(uint64_t rsp0) {
    memset((uint8_t *)&g_tss, 0, sizeof(g_tss));
    g_tss.rsp0 = rsp0;
    g_tss.iopb_offset = sizeof(tss64_t);

    g_kernel_rsp0 = rsp0;
}

void tss_install(void *gdt_base, uint16_t tss_selector) {
    uint64_t base = (uint64_t)&g_tss;
    uint32_t limit = (uint32_t)(sizeof(tss64_t) - 1);

    tss_desc_t *d = (tss_desc_t *)((uint8_t *)gdt_base + tss_selector);

    d->limit0 = limit & 0xFFFF;
    d->base0 = base & 0xFFFF;
    d->base1 = (base >> 16) & 0xFF;
    d->type = 0x89;
    d->limit1 = (limit >> 16) & 0x0F;
    d->base2 = (base >> 24) & 0xFF;
    d->base3 = (base >> 32) & 0xFFFFFFFF;
    d->reserved = 0;

    __asm__ __volatile__("ltr %0" : : "r"(tss_selector));
}

