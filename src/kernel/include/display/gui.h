#ifndef __DISPLAY_GUI
#define __DISPLAY_GUI

#include "../../../types.h"

typedef struct window {
    uint32_t wid;
    uint32_t x, y;
    uint32_t w, h;

    struct window *children;
} window_t;

void init_gui(void);
void update(void);
window_t new_window(void);


#endif

