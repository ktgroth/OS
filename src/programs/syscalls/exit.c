
#include "../include/syscalls/exit.h"

inline __attribute__((noreturn)) void sys_exit(uint64_t code) {
    __asm__ __volatile__ (
        "mov $60, %%rax\n\t"
        "syscall\n\t"
        :
        : "D"(code)
        : "rax", "rcx", "r11", "memory"
    );

    for (;;)
        __asm__ __volatile__("pause");
}

