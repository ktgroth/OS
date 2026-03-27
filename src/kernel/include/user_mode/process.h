#ifndef USER_PROCESS
#define USER_PROCESS

#include "../cpu/isr.h"
#include "../libc/types.h"


#define MAX_PROCESSES       64
#define PROCESS_NAME_LEN    32

typedef uint64_t (*app_entry_t)(void);

typedef enum {
    PROC_UNUSED = 0,
    PROC_RUNNABLE,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_ZOMBIE
} proc_state_e;

typedef struct process {
    uint64_t pid;
    uint64_t ppid;
    proc_state_e state;

    char name[PROCESS_NAME_LEN];

    registers_t regs;
    app_entry_t entry;

    uint64_t exit_code;
    uint8_t cancel_requested;
    uint8_t in_run_queue;

    uint64_t user_image_base;
    uint64_t user_stack_top;
} process_t;


process_t *create_user_process_from_image(const uint8_t *image,
                                          uint64_t image_size,
                                          uint64_t entry_off,
                                          const char *name);
void destroy_process(process_t *proc);

process_t *current_process(void);
void set_current_process(process_t *p);

void process_mark_exit(uint64_t code);

#endif

