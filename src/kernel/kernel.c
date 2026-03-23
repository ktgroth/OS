
#include "include/kernel.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/cpu/apic.h"
#include "include/cpu/timer.h"
#include "include/cpu/ports.h"
#include "include/driver/fat32.h"
#include "include/driver/storage.h"
#include "include/driver/vga.h"
#include "include/driver/syscall.h"
#include "include/driver/clock.h"
#include "include/libc/memory.h"
#include "include/libc/stdlib.h"
#include "include/libc/string.h"
#include "include/libc/printf.h"
#include "include/user_mode/scheduler.h"

#define HEAP_BASE 0xFFFF800000000000ULL
#define HEAP_PAGES 0x400

apic_info_t info;

static int64_t serial_ready() {
    return inb(COM1 + 5) & 0x20;
}

static void serial_write_char(char c) {
    while (!serial_ready());
    outb(COM1, c);
}

static void print_serial(const char *str) {
    while (*str) {
        if (*str == '\n')
            serial_write_char('\r');
        serial_write_char(*str++);
    }
}

int main() {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);

    isr_install();
    init_syscalls();
    init_memory();
    init_page_table();
    
    init_scheduler(0x1000)

    irq_install();

    if (!apic_discover(&info)) {
        printf("Could not discover APIC info.\n");
        __asm__ __volatile__("hlt");
    }

    apic_dump_info(&info);   
    uint64_t hz = get_cpu_hz(PIT_HZ, 50);
    printf("Clock Speed: %lu MHz\n", hz / 1000000ULL);

    init_kalloc(HEAP_BASE, HEAP_PAGES);
    init_bpb();
    init_serial();

    print_serial("KERNEL OK\n");

    printf("Type something, it will go through the kernel\n"
           "Type SHUTDOWN to Shutdown CPU\n> ");

    return 0;
}

