
#include "../include/cpu/ports.h"

typedef void (*ISR)(void);

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("out %%al, %%dx" : : "a" (data), "d" (port));
}

void io_wait(void) {
    volatile uint8_t *io_flag = (volatile uint8_t *)IO_FLAG_ADDR;

    while (*io_flag == 0)
        __asm__("hlt");

    *io_flag = 0;
}
