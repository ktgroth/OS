

#include "../include/cpu/isr.h"
#include "../include/driver/syscall.h"
#include "../include/driver/vga.h"
#include "../include/libc/types.h"
#include "../include/libc/string.h"


enum {
    SYS_WRITE = 1,
    SYS_EXIT = 2,
};

static void syscall_dispatch(registers_t *r) {
    switch (r->rax) {
        case SYS_WRITE: {
            const char *buf = (const char *)r->rdi;
            for (r->rax = 0; buf[r->rax]; ++r->rax)
                putchar(buf[r->rax], COLOR_WHT, COLOR_BLK);

            break;
        }

        case SYS_EXIT: {
            r->rax = r->rbx;
            return;
        }

        default:
            r->rax = (uint64_t)-1;
            break;
    }
}

void init_syscalls(void) {
    register_interrupt_handler(128, syscall_dispatch);
}

