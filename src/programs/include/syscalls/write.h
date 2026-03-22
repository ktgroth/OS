#ifndef __USER_SYSCALL_WRITE
#define __USER_SYSCALL_WRITE

#include "../libc/types.h"

uint64_t sys_write_str(long fd, const void *buf, uint64_t n);

#endif

