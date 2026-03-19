
#include "include/kernel.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/cpu/apic.h"
#include "include/cpu/timer.h"
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

apic_info_t info;

extern uint64_t tick;

int main() {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);

    isr_install();
    init_memory();
    init_page_table();

    if (!apic_discover(&info)) {
        printf("Could not discover APIC info.\n");
        __asm__ __volatile__("hlt");
    }

    apic_dump_info(&info);

    irq_install();
    init_syscalls();
    
    uint64_t hz = get_cpu_hz(PIT_HZ, 50);
    printf("Clock Speed: %lu MHz\n", hz / 1000000ULL);
    
    uint64_t t0 = tick;
    for (__volatile__ uint64_t i = 0; i < 300000000ULL; ++i) {}
    printf("tick delta=%lu\n", tick - t0);

    init_kalloc(HEAP_BASE, HEAP_PAGES);
    init_bpb();

    printf("Type something, it will go through the kernel\n"
           "Type SHUTDOWN to Shutdown CPU\n> ");

    return 0;
}

