
#include "../boot_info.h"
#include "include/display/framebuffer.h"

static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("hlt");
}

int kmain(boot_info_t *bi) {
    if (bi == 0 || bi->fb.base == 0 || bi->fb.width == 0 || bi->fb.height == 0)
        halt_forever();

    init_fb(bi->fb);
    fb_fill_rect(0, 0, bi->fb.width, bi->fb.height, 0x141414);
    fb_fill_rect(10, 10, 90, 90, 0xFFFFFF);
    fb_draw_line(100, 100, 200, 300, 0xF0F0F0);
    fb_draw_circ(200, 300, 16, 0xFFFFFF);

    halt_forever();
    return 0;
}

