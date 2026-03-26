
#include "../include/libc/stdlib.h"
#include "../include/libc/printf.h"
#include "../include/libc/memory.h"
#include "../include/user_mode/process.h"

#define KERNEL_CS   0x08
#define KERNEL_SS   0x10
#define USER_CS     0x1B
#define USER_SS     0x23

#define PAGE_PRESENT    0x01
#define PAGE_RW         0x02
#define PAGE_USER       0x04

#define USER_IMAGE_BASE 0x0000000000400000ULL
#define USER_STACK_TOP  0x0000000000800000ULL
#define USER_STACK_SIZE 0x0000000000004000ULL
#define PAGE_SIZE       0x1000ULL

static process_t g_process_table[MAX_PROCESSES];
static process_t *g_current_process = 0;
static uint64_t g_next_pid = 1;

static inline uint64_t page_down(uint64_t x) {
    return x & ~(PAGE_SIZE - 1);
}

static inline uint64_t page_up(uint64_t x) {
    return (x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static void process_trampoline(void) {
    process_t *p = current_process();
    uint64_t rc = 0;

    if (p && p->entry)
        rc = p->entry();

    process_mark_exit(rc);
    for (;;)
        __asm__ __volatile__("hlt");
}

static process_t *alloc_slot(void) {
    for (uint64_t i = 0; i < MAX_PROCESSES; ++i)
        if (g_process_table[i].state == PROC_UNUSED)
            return &g_process_table[i];

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

static int map_user_range(uint64_t va_start, uint64_t len, uint8_t flags) {
    uint64_t start = page_down(va_start);
    uint64_t end = page_up(va_start + len);
    for (uint64_t va = start; va < end; va += PAGE_SIZE) {
        void *pa = alloc_physical_page();
        if (!pa)
            return 0;
    
        map_page(pa, (void *)va, flags);
        memset((uint8_t *)va, 0, PAGE_SIZE);
    }

    return 1;
}

process_t *create_user_process_from_image(const uint8_t *image, uint64_t image_size, uint64_t entry_off, const char *name) {
    if (!image || image_size == 0 || entry_off >= image_size)
        return 0;

    process_t *p = alloc_slot();
    if (!p)
        return 0;

    memset((uint8_t *)p, 0, sizeof(*p));
    p->pid = g_next_pid++;
    p->state = PROC_RUNNABLE;
    p->cancel_requested = 0;
    p->in_run_queue = 0;
    copy_name(p->name, name);

    if (!map_user_range(USER_IMAGE_BASE, image_size, PAGE_PRESENT | PAGE_RW | PAGE_USER))
        return 0;
    if (!map_user_range(USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_SIZE, PAGE_PRESENT | PAGE_RW | PAGE_USER))
        return 0;

    memcpy((uint8_t *)USER_IMAGE_BASE, (uint8_t *)image, image_size);

    memset((uint8_t *)&p->regs, 0, sizeof(p->regs));
    p->regs.rip = USER_IMAGE_BASE + entry_off;
    p->regs.rsp = USER_STACK_TOP - 16;
    p->regs.cs = USER_CS;
    p->regs.ss = USER_SS;
    p->regs.eflags = 0x202ULL;

    printf("rip=%lx rsp=%lx\n", p->regs.rip, p->regs.rsp);

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

