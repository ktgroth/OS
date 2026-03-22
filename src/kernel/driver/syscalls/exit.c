
#include "../../include/driver/syscalls/exit.h"
#include "../../include/user_mode/process.h"
#include "../../include/user_mode/scheduler.h"

uint64_t sys_exit(uint64_t error_code) {
    process_t *p = current_process();
    if (!p)
        return error_code;

    p->exit_code = error_code;
    p->state = PROC_ZOMBIE;
    return error_code;
}

