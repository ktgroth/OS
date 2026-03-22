#ifndef __USER_SYSCALL_EXIT
#define __USER_SYSCALL_EXIT

#include "../libc/types.h"

__attribute__((noreturn)) void sys_exit(uint64_t code);

#endif

