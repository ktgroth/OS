
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/driver/vga.h"
#include "include/libc/mem.h"
#include "include/libc/stdlib.h"
#include "include/libc/string.h"
#include "include/kernel.h"

int main() {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);
    
    isr_install();
    irq_install();
    init_memory();

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
    } else if (!strcmp(input, "PAGE")) {
        uint64_t phys_addr;
        uint64_t page = malloc(1000, 1, &phys_addr);
        char page_str[66] = "";
        hex_to_ascii(page, page_str);

        char phys_str[66] = "";
        hex_to_ascii(phys_addr, phys_str);

        putstr("Page: ", COLOR_WHT, COLOR_BLK);
        putstr(page_str, COLOR_WHT, COLOR_BLK);
        putstr(", Physical Address: ", COLOR_WHT, COLOR_BLK);
        putstr(phys_str, COLOR_WHT, COLOR_BLK);
        putstr("\n> ", COLOR_WHT, COLOR_BLK);
    } else {
        putstr("You said: ", COLOR_WHT, COLOR_BLK);
        putstr(input, COLOR_WHT, COLOR_BLK);
        putstr("\n> ", COLOR_WHT, COLOR_BLK);
    }
}
