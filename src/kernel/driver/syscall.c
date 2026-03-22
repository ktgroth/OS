
#include "../include/cpu/isr.h"
#include "../include/driver/syscall.h"
#include "../include/driver/vga.h"
#include "../include/libc/types.h"
#include "../include/libc/printf.h"
#include "../include/libc/string.h"
#include "../include/user_mode/process.h"
#include "../include/user_mode/scheduler.h"

#include "../include/driver/syscalls/brk.h"
#include "../include/driver/syscalls/exec.h"
#include "../include/driver/syscalls/exit.h"
#include "../include/driver/syscalls/fork.h"
#include "../include/driver/syscalls/write.h"


enum {
    SYS_WRITE   = 0x01,
    SYS_MMAP    = 0x09,
    SYS_MUNMAP  = 0x0B,
    SYS_BRK     = 0x0C,
    SYS_FORK    = 0x39,
    SYS_EXECVE  = 0x3B,
    SYS_EXIT    = 0x3C,
    SYS_WAIT4   = 61,
};

static void syscall_dispatch(registers_t *r) {
    switch (r->rax) {
        case SYS_WRITE:
            r->rax = sys_write(r->rdi, (const char *)r->rsi, r->rdx);
            break;

        // case SYS_MMAP: {
        //     r->rax = sys_mmap(r->rdi, r->rsi, r->rdx, r->r10, r->r8, r->r9);
        //     break;
        // }
    
        // case SYS_MUNMAP: {
        //     r->rax = sys_munmap(r->rdi, r->rsi);
        //     break;
        // }

        case SYS_BRK:
            r->rax = sys_brk(r->rdi);
            break;

        case SYS_FORK:
            r->rax = sys_fork();
            break;

        case SYS_EXECVE:
            r->rax = sys_execve((const char *)r->rdi, (char *const *)r->rsi, (const char *const *)r->rdx);
            break;

        case SYS_EXIT:
            r->rax = sys_exit(r->rdi);
            scheduler_on_tick(r);
            break;

        // case SYS_WAIT4: {
        //     r->rax = sys_wait4((int)r->rdi, (int *)r->rsi, (int)r->rdx);
        //     break;
        // }

        default:
            printf("Unknown syscall '%d'\n", r->rax);
            break;
    }
    
    return;
}

void init_syscalls(void) {
    register_interrupt_handler(128, syscall_dispatch);
}

