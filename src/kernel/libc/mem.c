
#include "../include/driver/vga.h"
#include "../include/libc/mem.h"

#include "../include/libc/string.h"

extern uint64_t DETECTED_MEMORY;

mmap_t *mbuffer[256];
uint64_t free_mem_addr = 0x10000;

void init_memory() {
    mmap_t *buffer = (mmap_t *)&DETECTED_MEMORY;
    uint32_t i = 0;
    while (buffer[i].base + buffer[i++].length) {
        uint64_t base = buffer[i].base;
        uint64_t length = buffer[i].length;
        uint32_t type = buffer[i].type;
        uint32_t ACPI = buffer[i].ACPI;
        
        char base_str[66] = "";
        char length_str[32] = "";
        char type_str[2] = "";

        hex_to_ascii(base, base_str);
        hex_to_ascii(length, length_str);
        int_to_ascii(type, type_str);

        putstr("BASE: ", COLOR_WHT, COLOR_BLK);
        putstr(base_str, COLOR_WHT, COLOR_BLK);
        putstr(", LENGTH: ", COLOR_WHT, COLOR_BLK);
        putstr(length_str, COLOR_WHT, COLOR_BLK);
        putstr(", TYPE: ", COLOR_WHT, COLOR_BLK);
        putstr(type_str, COLOR_WHT, COLOR_BLK);
        putstr("\n", COLOR_WHT, COLOR_BLK);
    }
}

uint64_t malloc(uint64_t size, int align, uint64_t *phys_addr) {
    if (align == 1 && (free_mem_addr & 0xFFFFF000)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }

    if (phys_addr)
        *phys_addr = free_mem_addr;

    uint64_t ret = free_mem_addr;
    free_mem_addr += size;
    return ret;
}
