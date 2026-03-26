#ifndef __DRIVER_MOUSE_H
#define __DRIVER_MOUSE_H

#include "../libc/types.h"

typedef struct {
    int32_t x;
    int32_t y;
    int8_t dx;
    int8_t dy;
    uint8_t left;
    uint8_t right;
    uint8_t middle;
    uint8_t changed;
} mouse_state_t;

void init_mouse(void);
void mouse_set_bounds(uint32_t width, uint32_t height);
void mouse_get_state(mouse_state_t *out);

#endif

