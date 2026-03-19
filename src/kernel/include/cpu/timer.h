
#ifndef __CPU_TIMER
#define __CPU_TIMER

#include "../libc/types.h"

#define PIT_HZ 0x1000

extern uint64_t get_cpu_hz(uint32_t freq, uint32_t sample_ticks);
void init_timer(uint32_t freq);

#endif
