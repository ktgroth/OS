#ifndef USER_SCHEDULER
#define USER_SCHEDULER

#include "../cpu/isr.h"
#include "../user_mode/process.h"
#include "../libc/types.h"

void init_scheduler(uint32_t quantum_ticks);
void scheduler_enqueue(process_t *p);
void scheduler_on_tick(registers_t *frame);
void scheduler_cancel_current(void);

#endif

