
#include "../include/syscalls/exit.h"

inline __attribute__((noreturn)) void sys_exit(uint64_t code) {
    __asm__ __volatile__ (
        "mov $60, %%rax\n\t"
        "int $0x80\n\t"
        :
        : "D"(code)
        : "rax", "memory"
    );

    for (;;)
        __asm__ __volatile__("hlt");
}

