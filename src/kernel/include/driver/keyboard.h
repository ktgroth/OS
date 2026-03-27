
#ifndef __DRIVER_KEYBOARD
#define __DRIVER_KEYBOARD

#include "../libc/types.h"

void init_keyboard();
uint8_t keyboard_try_get_line(char *out, uint64_t out_sz);

#endif
