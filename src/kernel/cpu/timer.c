
#include "../include/cpu/isr.h"
#include "../include/cpu/ports.h"
#include "../include/cpu/timer.h"
#include "../include/driver/vga.h"
#include "../include/libc/string.h"
#include "../include/libc/function.h"

uint32_t tick = 0;

static void timer_callback(registers_t regs) {
    ++tick;
    UNUSED(regs);
}

void init_timer(uint32_t freq) {
    register_interrupt_handler(IRQ0, timer_callback);

    uint32_t divisor = 1193180 / freq;
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(0x43, 0x36);
    outb(0x43, low);
    outb(0x43, high);
}
