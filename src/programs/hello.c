
#include "include/libc/types.h"
#include "include/syscalls/write.h"
#include "include/syscalls/exit.h"

__attribute__((section(".text.entry")))
uint64_t _start(void) {
    for (uint64_t i = 0; i < 0x10000000; ++i);
    static const char msg[] = "HELLO WORLD\n";
    sys_write_str(1, msg, sizeof(msg) - 1);
    sys_exit(0);
}

