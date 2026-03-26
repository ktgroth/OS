#ifndef __CPU_GDT
#define __CPU_GDT

#include "../libc/types.h"

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtr_t;

#define GDT_SEL_KCODE   0x08
#define GDT_SEL_KDATA   0x10
#define GDT_SEL_UCODE   0x1B
#define GDT_SEL_UDATA   0x23
#define GDT_SEL_TSS     0x28

extern uint64_t gdt_runtime[];
extern gdtr_t gdt_runtime_ptr;

void gdt_runtime_load(void);

#endif

