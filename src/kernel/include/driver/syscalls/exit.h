#ifndef DRIVER_SYSCALL_EXIT
#define DRIVER_SYSCALL_EXIT

#include "../../cpu/isr.h"

void sys_exit(uint64_t error_code);

#endif

