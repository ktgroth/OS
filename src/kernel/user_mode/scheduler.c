
#include "../include/user_mode/scheduler.h"
#include "../include/libc/printf.h"

#define USER_CS 0x1B
#define USER_SS 0x23

static process_t *g_runq[MAX_PROCESSES];
static uint32_t g_q_head = 0;
static uint32_t g_q_tail = 0;
static uint32_t g_q_count = 0;

static uint32_t g_quantum_ticks = 5;
static uint32_t g_ticks_in_slice = 0;
static uint32_t g_force_resched = 0;

static volatile uint8_t g_sched_active = 0;

static registers_t g_kernel_frame;
static uint8_t g_have_kernel_frame = 0;

static uint8_t frame_user_sane(const registers_t *r) {
    return r &&
        r->cs == 0x1B &&
        r->ss == 0x23 &&
        r->rip >= 0x400000 && r->rip < 0x800000 &&
        r->rsp >= 0x500000 && r->rsp < 0x900000;
}

static uint8_t frame_kernel_sane(const registers_t *r) {
    return r &&
        r->cs == 0x08 &&
        r->ss == 0x10 &&
        r->rip >= 0xC800 && r->rip < 0x200000 &&
        r->rsp >= 0x80000 && r->rsp < 0x90000;
}

static void dump_frame(const char *tag, const registers_t *r) {
    if (!r)
        return;

    printf("%s rip=%lx cs=%lx rfl=%lx rsp=%lx ss=%lx irq=%lx err=%lx\n",
            tag, r->rip, r->cs, r->eflags, r->rsp, r->ss, r->irq_number, r->error_code);
}

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

extern void enter_user(uint64_t rip, uint64_t rsp);

static void dispatch(process_t *next, registers_t *frame) {
    if (!next || !frame)
        return;

    dump_frame("DISPATCH-BEFORE", frame);

    next->regs.cs = USER_CS;
    next->regs.ss = USER_SS;
    next->regs.eflags |= 0x202ULL;

    next->state = PROC_RUNNING;
    set_current_process(next);
    dump_frame("DISPATCH-NEXT", &next->regs);
    *frame = next->regs;
    dump_frame("DISPATCH-AFTER", frame);

    g_ticks_in_slice = 0;
    g_force_resched = 0;  
}

void scheduler_capture_kernel_frame(registers_t *frame) {
    if (!frame)
        return;
    if ((frame->cs & 0x03) != 0x00)
        return;
    if (current_process())
        return;
    if (g_have_kernel_frame)
        return;

    g_kernel_frame = *frame;
    g_have_kernel_frame = 1;
    dump_frame("KF-SAVE", frame);
}

void init_scheduler(uint32_t quantum_ticks) {
    uint64_t f = irq_save_disable();

    if (quantum_ticks == 0)
        quantum_ticks = 5;

    g_q_head = g_q_tail = g_q_count = 0;
    g_quantum_ticks = quantum_ticks;
    g_ticks_in_slice = 0;
    g_force_resched = 0;
    g_sched_active = 0;
    g_have_kernel_frame = 0;

    for (uint32_t i = 0; i < MAX_PROCESSES; ++i)
        g_runq[i] = 0;

    irq_restore(f);
}

void scheduler_enqueue(process_t *p) {
    if (!p)
        return;
    if (p->state == PROC_ZOMBIE || p->state == PROC_UNUSED)
        return;

    uint64_t f = irq_save_disable();
    p->state = PROC_RUNNABLE;
    enqueue_nolock(p);
    irq_restore(f);
}

void scheduler_cancel_current(void) {
    uint64_t f = irq_save_disable();

    process_t *p = current_process();
    if (p) {
        p->cancel_requested = 1;
        g_force_resched = 1;
    }

    irq_restore(f);
}

void scheduler_start_if_idle(void) {
    uint64_t f = irq_save_disable();
    if (g_sched_active || current_process() || g_q_count == 0) {
        irq_restore(f);
        return;
    }

    process_t *next = next_runnable_nolock();
    if (!next) {
        irq_restore(f);
        return;
    }

    next->regs.cs = USER_CS;
    next->regs.ss = USER_SS;
    next->regs.eflags |= 0x202ULL;
    next->state = PROC_RUNNING;
    set_current_process(next);

    uint64_t rip = next->regs.rip;
    uint64_t rsp = next->regs.rsp;

    g_ticks_in_slice = 0;
    g_force_resched = 0;

    irq_restore(f);
    enter_user(rip, rsp);
}

void scheduler_on_tick(registers_t *frame) {
    if (!frame)
        return;

    if ((frame->cs & 0x03) != 0x03)
        return;

    uint64_t f = irq_save_disable();
    if (g_sched_active) {
        irq_restore(f);
        return;
    }
    g_sched_active = 1;

    process_t *curr = current_process();
    if (!curr) {
        process_t *next = next_runnable_nolock();
        if (next)
            dispatch(next, frame);
        goto out;
    }
    
    if (!g_force_resched && curr->state == PROC_RUNNING && !curr->cancel_requested) {
        g_ticks_in_slice++;
        if (g_ticks_in_slice < g_quantum_ticks)
            goto out;
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
    }

    process_t *next = next_runnable_nolock();
    if (next)
        dispatch(next, frame);
    else if (!current_process() && g_have_kernel_frame)
        *frame = g_kernel_frame;

out:
    g_sched_active = 0;
    irq_restore(f);
}

