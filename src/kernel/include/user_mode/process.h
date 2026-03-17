#ifndef USER_PROCESS
#define USER_PROCESS

#include "../cpu/isr.h"
#include "../libc/memory.h"
#include "../libc/types.h"


#define MAX_PROCESSES       64
#define PROCESS_NAME_LEN    32

#define PAGE_SIZE           0x1000ULL
#define USER_CODE_BASE      0x0000000000400000ULL
#define USER_STACK_TOP      0x0000000000800000ULL
#define USER_STACK_PAGES    8ULL
#define USER_HEAP_BASE      0x0000000000900000ULL
#define USER_HEAP_MAX_PAGES 256ULL


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

    registers_t *call_frame;
    registers_t *sregs;

    pml4_t *image_base;
    uint64_t image_size;

    uint64_t brk_start;
    uint64_t brk_end;
    uint64_t brk_limit;
} process_t;


process_t *create_new_process();
void destroy_process(process_t *proc);

process_t *current_process(void);
void set_current_process(process_t *p);

#endif

