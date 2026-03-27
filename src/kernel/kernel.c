
#include "include/kernel.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/cpu/apic.h"
#include "include/cpu/timer.h"
#include "include/cpu/ports.h"
#include "include/cpu/tss.h"
#include "include/cpu/gdt.h"
#include "include/driver/fat32.h"
#include "include/driver/storage.h"
#include "include/driver/vga.h"
#include "include/driver/syscall.h"
#include "include/driver/clock.h"
#include "include/driver/framebuffer.h"
#include "include/driver/keyboard.h"
#include "include/driver/mouse.h"
#include "include/driver/gui.h"
#include "include/libc/memory.h"
#include "include/libc/stdlib.h"
#include "include/libc/string.h"
#include "include/libc/printf.h"
#include "include/user_mode/scheduler.h"

#define HEAP_BASE 0xFFFF800000000000ULL
#define HEAP_PAGES 0x400
#define KSTACK_SIZE 0x4000

apic_info_t info;
extern uint8_t fb_ready;
static uint8_t g_kernel_boot_stack[KSTACK_SIZE] __attribute__((aligned(16)));

int main() {
    gdt_runtime_load();
    
    tss_init((uint64_t)&g_kernel_boot_stack[KSTACK_SIZE]);
    tss_install((void *)&gdt_runtime[0], GDT_SEL_TSS);

    fb_ready = fb_init_from_bootinfo();
    
    if (fb_ready) {
        framebuffer_t *fb = fb_get();
        gui_draw_desktop();
        gui_cursor_init();
        mouse_set_bounds(fb->width, fb->height);
        init_serial();
        printf("Serial: Online\n");
    } else {
        clearwin(COLOR_WHT, COLOR_BLK);
        printf("Framebuffer init failed. Using VGA text mode.\n");
    }

    isr_install();
    init_syscalls();
    init_memory();
    init_page_table();

    if (!apic_discover(&info)) {
        printf("APIC_DISCOVER FAILED\n");
        __asm__ __volatile__("hlt");
    }

    printf("APIC_DISCOVER OK\n");
    irq_install();
    printf("IRQ_INSTALL OK\n");

    apic_dump_info(&info);
    printf("APIC DUMPED OK\n");
    uint64_t hz = get_cpu_hz(PIT_HZ, 50);
    printf("Clock Speed: %lu MHz\n", hz / 1000000ULL);

    init_kalloc(HEAP_BASE, HEAP_PAGES);
    printf("INIT_KALLOC OK\n");
    init_bpb();
    printf("INIT_BPP OK\n");
    
    init_scheduler(0x1000);
    printf("INIT_SCHEDULER OK\n");

    printf("> ");
    for (;;) {
        char line[256];
        if (keyboard_try_get_line(line, sizeof(line)))
            user_input(line);

        __asm__ __volatile__("hlt");
    }

    return 0;
}

