
#ifndef __LIBC_MEM
#define __LIBC_MEM

#include "types.h"

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t ACPI;
} mmap_t;

enum E820_memory_block_type {
    E820_USABLE = 1,
    E820_RESERVED,
    E820_ACPI_RECLAIMABLE,
    E820_ACPI_NVS,
    E820_BAD
};

void init_memory();
uint64_t malloc(uint64_t size, int align, uint64_t *phys_addr);

#endif
