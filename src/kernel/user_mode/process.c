
#include "../include/libc/stdlib.h"
#include "../include/libc/printf.h"
#include "../include/user_mode/process.h"

#define KERNEL_CS   0x08
#define KERNEL_SS   0x10

static process_t g_process_table[MAX_PROCESSES];
static uint8_t g_proc_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];
static process_t *g_current_process = 0;
static uint64_t g_next_pid = 1;

static void process_trampoline(void) {
    process_t *p = current_process();
    uint64_t rc = 0;

    if (p && p->entry)
        rc = p->entry();

    process_mark_exit(rc);
    for (;;)
        __asm__ __volatile__("hlt");
}

static process_t *alloc_slot(uint64_t *slot_out) {
    for (uint64_t i = 0; i < MAX_PROCESSES; ++i) {
        if (g_process_table[i].state == PROC_UNUSED) {
            if (slot_out)
                *slot_out = i;
            return &g_process_table[i];
        }
    }

    return 0;
}

static void copy_name(char dst[PROCESS_NAME_LEN], const char *src) {
    uint64_t i = 0;
    if (!src) {
        dst[0] = '\0';
        return;
    }

    for (; src[i] && i < PROCESS_NAME_LEN - 1; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

process_t *create_new_process(void) {
    uint64_t slot = 0;
    process_t *p = alloc_slot(&slot);
    if (!p)
        return 0;

    memset((uint8_t *)p, 0, sizeof(*p));
    p->pid = g_next_pid++;
    p->state = PROC_RUNNABLE;

    return p;
}

process_t *create_process(app_entry_t entry, const char *name) {
    uint64_t slot = 0;
    process_t *p = alloc_slot(&slot);
    if (!p)
        return 0;

    memset((uint8_t *)p, 0, sizeof(*p));

    p->pid = g_next_pid++;
    p->state = PROC_RUNNABLE;
    p->entry = entry;
    p->cancel_requested = 0;
    p->in_run_queue = 0;
    copy_name(p->name, name);

    uint64_t stack_top = (uint64_t)&g_proc_stacks[slot][PROCESS_STACK_SIZE];
    stack_top &= ~0xFULL;

    memset((uint8_t *)&p->regs, 0, sizeof(p->regs));
    p->regs.rip = (uint64_t)process_trampoline;
    p->regs.rsp = stack_top;
    p->regs.cs = KERNEL_CS;
    p->regs.ss = KERNEL_SS;
    p->regs.eflags = 0x202ULL;

    return p;
}

void destroy_process(process_t *proc) {
    if (!proc)
        return;

    memset((uint8_t *)proc, 0, sizeof(*proc));
    proc->state = PROC_UNUSED;
}

process_t *current_process(void) {
    return g_current_process;
}

void set_current_process(process_t *p) {
    g_current_process = p;
}

void process_mark_exit(uint64_t code) {
    process_t *p = current_process();
    if (!p)
        return;

    p->exit_code = code;
    p->state = PROC_ZOMBIE;
}

