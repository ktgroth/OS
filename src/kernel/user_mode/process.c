
#include "../include/user_mode/process.h"

static process_t *g_current_process = 0;

process_t *current_process(void) {
    return g_current_process;
}

void set_current_process(process_t *p) {
    g_current_process = p;
}

