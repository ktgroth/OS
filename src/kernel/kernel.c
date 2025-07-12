
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/driver/vga.h"
#include "include/driver/fat32.h"
#include "include/driver/storage.h"
#include "include/libc/memory.h"
#include "include/libc/stdlib.h"
#include "include/libc/string.h"
#include "include/kernel.h"

extern void *alloc_physical_page();

#define HEAP_BASE 0xFFFF800000000000ULL
#define HEAP_PAGES 0x400

int main() {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);

    isr_install();
    irq_install();
    init_memory();
    init_page_table();
    init_kalloc(HEAP_BASE, HEAP_PAGES);
    init_bpb();
    init_fats_root();

    putstr("Type something, it will go through the kernel\n"
           "Type END to halt the CPU\n> ", COLOR_WHT, COLOR_BLK);

    return 0;
}

void user_input(char *input) {
    if (!strcmp(input, "CLEAR")) {
        set_cursor_pos(0, 0);
        clearwin(COLOR_WHT, COLOR_BLK);

        putstr("Type something, it will go through the kernel\n"
               "Type END to halt the CPU\n> ", COLOR_WHT, COLOR_BLK);
    } else if (!strcmp(input, "END")) {
        putstr("Stopping the CPU.\n", COLOR_WHT, COLOR_BLK);
        hide_cursor();
        __asm__ __volatile__("hlt");
    } else if (!strncmp(input, "ALLOC", strlen("ALLOC"))) {
        uint32_t *virt_addr = (uint32_t *)kmalloc(0x1000);
        uint64_t *phys_addr = (uint64_t *)get_paddr(virt_addr);
        char virt_str[66] = "";
        hex_to_ascii((uint64_t)virt_addr, virt_str);

        char phys_str[66] = "";
        hex_to_ascii((uint64_t)phys_addr, phys_str);

        putstr("VAddr: ", COLOR_WHT, COLOR_BLK);
        putstr(virt_str, COLOR_WHT, COLOR_BLK);
        putstr(", PAddr: ", COLOR_WHT, COLOR_BLK);
        putstr(phys_str, COLOR_WHT, COLOR_BLK);
        putstr("\n> ", COLOR_WHT, COLOR_BLK);
    } else if (!strcmp(input, "PWD")) {
        char *cwd = getcwd();
        putstr(cwd, COLOR_WHT, COLOR_BLK);
        putstr("\n> ", COLOR_WHT, COLOR_BLK);
    } else {
        putstr("You said: ", COLOR_WHT, COLOR_BLK);
        putstr(input, COLOR_WHT, COLOR_BLK);
        putstr("\n> ", COLOR_WHT, COLOR_BLK);
    }
}
