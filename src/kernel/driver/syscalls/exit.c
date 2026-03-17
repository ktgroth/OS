
#include "../../include/driver/syscalls/exit.h"
#include "../../include/user_mode/process.h"

extern void schedule(void);

void sys_exit(uint64_t error_code) {
    process_t *p = current_process();
    if (!p)
        for (;;)
            __asm__ __volatile__("hlt");

    p->state = PROC_ZOMBIE;
    schedule();

    for (;;)
        __asm__ __volatile__("hlt");
}

