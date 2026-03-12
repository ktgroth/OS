#ifndef DRIVER_SYSCALL_WRITE
#define DRIVER_SYSCALL_WRITE

#include "../../libc/types.h"
#include "../../cpu/isr.h"

int64_t sys_write(uint64_t fd, const char *buf, uint64_t count);

#endif

