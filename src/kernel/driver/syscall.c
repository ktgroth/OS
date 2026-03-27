
#include "../include/cpu/isr.h"
#include "../include/driver/syscall.h"
#include "../include/libc/types.h"
#include "../include/libc/printf.h"
#include "../include/user_mode/process.h"
#include "../include/user_mode/scheduler.h"

#include "../include/driver/syscalls/brk.h"
#include "../include/driver/syscalls/exec.h"
#include "../include/driver/syscalls/exit.h"
#include "../include/driver/syscalls/fork.h"
#include "../include/driver/syscalls/write.h"


#define IA32_EFER   0xC0000080
#define IA32_STAR   0xC0000081
#define IA32_LSTAR  0xC0000082
#define IA32_FMASK  0xC0000084

#define KERNEL_CS   0x08
#define USER_CS     0x1B

extern void syscall_entry(void);

typedef struct __attribute__((packed)) {
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rax;
    uint64_t rcx;
    uint64_t r11;
} syscall_frame_t;

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t v) {
    uint32_t lo = (uint32_t)v;
    uint32_t hi = (uint32_t)(v >> 32);
    __asm__ __volatile__("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

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

uint64_t syscall_dispatch(syscall_frame_t *f) {
    switch (f->rax) {
        case SYS_WRITE:
            f->rax = sys_write(f->rdi, (const char *)f->rsi, f->rdx);
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
            f->rax = sys_brk(f->rdi);
            break;

        case SYS_FORK:
            f->rax = sys_fork();
            break;

        case SYS_EXECVE:
            f->rax = sys_execve((const char *)f->rdi, (char *const *)f->rsi, (const char *const *)f->rdx);
            break;

        case SYS_EXIT:
            f->rax = sys_exit(f->rdi);
            break;

        // case SYS_WAIT4: {
        //     r->rax = sys_wait4((int)r->rdi, (int *)r->rsi, (int)r->rdx);
        //     break;
        // }

        default:
            printf("Unknown syscall '%d'\n", f->rax);
            break;
    }
    
    return -1;
}

void init_syscalls(void) {
    uint64_t efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | 1ULL);

    uint64_t star =
        (((uint64_t)(USER_CS - 16)) << 48) |
        (((uint64_t)KERNEL_CS) << 32);

    wrmsr(IA32_STAR, star);
    wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);
    wrmsr(IA32_FMASK, (1ULL << 9));
}

