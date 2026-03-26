
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

uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

void io_wait(void) {
    volatile uint8_t *io_flag = (volatile uint8_t *)IO_FLAG_ADDR;
    while (*io_flag == 0);
    *io_flag = 0;
}

static uint8_t serial_tx_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

void init_serial(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_putc(char c) {
    while (!serial_tx_ready());
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s) {
    if (!s)
        return;

    while (*s) {
        if (*s == '\n')
            serial_putc('\r');
        serial_putc(*s++);
    }
}

