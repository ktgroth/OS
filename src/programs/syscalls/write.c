
#include "../include/syscalls/write.h"

inline uint64_t sys_write_str(long fd, const void *buf, uint64_t n) {
    uint64_t ret;
    __asm__ __volatile__ (
        "mov $1, %%rax\n\t"
        "int $0x80\n\t"
        : "=a"(ret)
        : "D"(fd), "S"(buf), "d"(n)
        : "memory"
    );
    return ret;
}

