#ifndef DRIVER_SYSCALL_BRK
#define DRIVER_SYSCALL_BRK

#include "../../libc/types.h"

uint64_t sys_brk(uint64_t new_end);

#endif

