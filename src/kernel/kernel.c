
#include "include/kernel.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/driver/fat32.h"
#include "include/driver/storage.h"
#include "include/driver/vga.h"
#include "include/driver/syscall.h"
#include "include/driver/clock.h"
#include "include/libc/memory.h"
#include "include/libc/stdlib.h"
#include "include/libc/string.h"
#include "include/libc/printf.h"

#define HEAP_BASE 0xFFFF800000000000ULL
#define HEAP_PAGES 0x400

#define NULL ((void *)0x0)

int main() {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);

    isr_install();
    init_syscalls();
    irq_install();
    init_memory();
    init_page_table();
    init_kalloc(HEAP_BASE, HEAP_PAGES);
    init_bpb();

    printf("Type something, it will go through the kernel\n"
           "Type SHUTDOWN to Shutdown CPU\n> ");

    return 0;
}

