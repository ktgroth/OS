#ifndef DRIVER_SYSCALL_EXIT
#define DRIVER_SYSCALL_EXIT

#include "../../libc/types.h"

uint64_t sys_exit(uint64_t error_code);

#endif

