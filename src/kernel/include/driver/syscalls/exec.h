#ifndef DRIVER_SYSCALL_EXEC
#define DRIVER_SYSCALL_EXEC

#include "../../libc/types.h"

int64_t sys_execve(const char *filename, const char *const argv[], const char *const *envp);

#endif

