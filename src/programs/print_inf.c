
#include "include/libc/types.h"
#include "include/syscalls/write.h"
#include "include/syscalls/exit.h"

__attribute__((section(".text.entry")))
uint64_t _start(void) {
    static const char msg[] = "INF PRINT PROGRAM\n";
    while (1) {
        sys_write_str(1, msg, sizeof(msg) - 1);
        for (uint64_t i = 0; i < 0x100000000; ++i);
    }

    sys_exit(0);
}

