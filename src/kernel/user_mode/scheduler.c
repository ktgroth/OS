
#include "../include/user_mode/scheduler.h"
#include "../include/libc/printf.h"

static process_t *g_runq[MAX_PROCESSES];
static uint32_t g_q_head = 0;
static uint32_t g_q_tail = 0;
static uint32_t g_q_count = 0;

static uint32_t g_quantum_ticks = 5;
static uint32_t g_ticks_in_slice = 0;
static uint32_t g_force_resched = 0;

static registers_t g_kernel_frame;
static uint8_t g_have_kernel_frame = 0;

static inline uint64_t irq_save_disable(void) {
    uint64_t flags;
    __asm__ __volatile__("pushfq; popq %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint64_t flags) {
    if (flags & (1ULL << 9))
        __asm__ __volatile__("sti" ::: "memory");
}

static uint8_t enqueue_nolock(process_t *p) {
    if (!p)
        return 0;
    if (p->in_run_queue)
        return 1;
    if (g_q_count >= MAX_PROCESSES)
        return 0;

    g_runq[g_q_tail] = p;
    g_q_tail = (g_q_tail + 1) % MAX_PROCESSES;
    g_q_count++;
    p->in_run_queue = 1;

    // printf("ENQUEUE:\n HEAD: %lu\n TAIL: %lu\n", g_q_head, g_q_tail);
    return 1;
}

static process_t *dequeue_nolock(void) {
    if (g_q_count == 0)
        return 0;

    process_t *p = g_runq[g_q_head];
    g_q_head = (g_q_head + 1) % MAX_PROCESSES;
    g_q_count--;
    if (p)
        p->in_run_queue = 0;

    // printf("DEQUEUE:\n HEAD: %lu\n TAIL: %lu\n", g_q_head, g_q_tail);
    return p;
}

static process_t *next_runnable_nolock(void) {
    uint32_t scans = g_q_count;
    while (scans--) {
        process_t *p = dequeue_nolock();
        if (!p)
            continue;

        if (p->state == PROC_ZOMBIE || p->state == PROC_UNUSED) {
            destroy_process(p);
            continue;
        }

        if (p->state == PROC_RUNNABLE)
            return p;

        enqueue_nolock(p);
    }

    return 0;
}

void init_scheduler(uint32_t quantum_ticks) {
    if (quantum_ticks == 0)
        quantum_ticks = 5;

    g_q_head = g_q_tail = g_q_count = 0;
    g_quantum_ticks = quantum_ticks;
    g_ticks_in_slice = 0;
    g_force_resched = 0;
    g_have_kernel_frame = 0;

    for (uint32_t i = 0; i < MAX_PROCESSES; ++i)
        g_runq[i] = 0;
}

void scheduler_enqueue(process_t *p) {
    if (!p)
        return;
    if (p->state == PROC_ZOMBIE || p->state == PROC_UNUSED)
        return;

    uint64_t flags = irq_save_disable();
    p->state = PROC_RUNNABLE;
    enqueue_nolock(p);
    irq_restore(flags);
}

void scheduler_cancel_current(void) {
    process_t *p = current_process();
    if (!p)
        return;

    p->cancel_requested = 1;
    g_force_resched = 1;
}

static void dispatch(process_t *next, registers_t *frame) {
    if (!next || !frame)
        return;

    next->state = PROC_RUNNING;
    set_current_process(next);
    *frame = next->regs;
    g_ticks_in_slice = 0;
    g_force_resched = 0;
    return;
}

void scheduler_on_tick(registers_t *frame) {
    if (!frame)
        return;

    process_t *curr = current_process();
    if (!curr) {
        process_t *next = next_runnable_nolock();
        if (!next)
            return;

        g_kernel_frame = *frame;
        g_have_kernel_frame = 1;

        dispatch(next, frame);
        return;
    }
    
    if (!g_force_resched && curr->state == PROC_RUNNING && !curr->cancel_requested) {
        g_ticks_in_slice++;
        if (g_ticks_in_slice < g_quantum_ticks)
            return;
    }

    g_ticks_in_slice = 0;
    g_force_resched = 0;

    curr->regs = *frame;
    if (curr->state == PROC_RUNNING) {
        if (curr->cancel_requested)
            curr->state = PROC_ZOMBIE;
        else
            curr->state = PROC_RUNNABLE;
    }

    if (curr->state == PROC_RUNNABLE)
        enqueue_nolock(curr);
    else if (curr->state == PROC_ZOMBIE) {
        set_current_process(0);
        destroy_process(curr);
        curr = 0;
    }

    process_t *next = next_runnable_nolock();
    if (next) {
        dispatch(next, frame);
        return;
    }

    set_current_process(0);
    if (g_have_kernel_frame)
        *frame = g_kernel_frame;
}

