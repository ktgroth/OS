#ifndef __CPU_TSS
#define __CPU_TSS

#include "../libc/types.h"

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss64_t;

typedef struct __attribute__((packed)) {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t type;
    uint8_t limit1;
    uint8_t base2;
    uint32_t base3;
    uint32_t reserved;
} tss_desc_t;

extern uint64_t g_kernel_rsp0;

void tss_init(uint64_t rsp0);
void tss_install(void *gdt_base, uint16_t tss_selector);

#endif

