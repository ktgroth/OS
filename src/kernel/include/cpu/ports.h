
#ifndef __CPU_PORTS
#define __CPU_PORTS

#include "../libc/types.h"

#define IO_FLAG_ADDR    0x5000
#define INT_ENABLE_ADDR 0x4000
#define INT_VECTOR_ADDR 0x0000
#define IO_COMPLETE_INT 0x01
#define COM1            0x3F8

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);

uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

void io_wait(void);
void init_serial(void);
void serial_putc(char c);
void serial_puts(const char *s);

#endif
