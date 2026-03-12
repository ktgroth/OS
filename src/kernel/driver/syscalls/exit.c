
#include "../../include/driver/syscalls/exit.h"

void sys_exit(registers_t *r) {
    r->rax = r->rdi;
}

