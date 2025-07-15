
#include "../include/cpu/isr.h"
#include "../include/cpu/ports.h"
#include "../include/cpu/timer.h"
#include "../include/driver/vga.h"
#include "../include/libc/string.h"
#include "../include/libc/function.h"

#define CHANNEL_0   0x40
#define CHANNEL_1   0x41
#define CHANNEL_2   0x42
#define COMMAND     0x43

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

    outb(COMMAND, 0x36);
    outb(COMMAND, low);
    outb(COMMAND, high);
}

uint64_t read_pit_count() {
    __asm__ __volatile__("cli");

    outb(COMMAND, 0b00000000);

    tick = inb(CHANNEL_0);
    tick |= inb(CHANNEL_0) << 8;

    return tick;
}

void set_pit_count() {
    __asm__ __volatile__("cli");

    outb(CHANNEL_0, tick & 0xFF);
    outb(CHANNEL_0, (tick & 0xFF00) >> 8);
}

void sleep(uint64_t millis) {
    while (millis > 0) {
        __asm__ __volatile__("hlt");
    }
}

