
#ifndef __CPU_PORTS
#define __CPU_PORTS

#include "../libc/types.h"

#define IO_FLAG_ADDR    0x5000
#define INT_ENABLE_ADDR 0x4000
#define INT_VECTOR_ADDR 0x0000
#define IO_COMPLETE_INT 0x01

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);

#endif
