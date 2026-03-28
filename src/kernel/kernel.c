
#include "../boot_info.h"

static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("hlt");
}

static uint32_t make_pixel(uint8_t r, uint8_t g, uint8_t b, uint32_t pixel_format) {
    if (pixel_format == 1)
        return ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int kmain(boot_info_t *bi) {
    if (bi == 0 || bi->fb.base == 0 || bi->fb.width == 0 || bi->fb.height == 0)
        halt_forever();

    volatile uint32_t *fb = (volatile uint32_t *)(uint64_t)bi->fb.base;
    uint32_t w = bi->fb.width;
    uint32_t h = bi->fb.height;
    uint32_t stride = bi->fb.ppl;

    uint32_t bg = make_pixel(0x10, 0x60, 0x10, bi->fb.format);
    uint32_t fg = make_pixel(0xFF, 0xFF, 0xFF, bi->fb.format);

    for (uint32_t y = 0; y < h; ++y)
        for (uint32_t x = 0; x < w; ++x)
            fb[y * stride + x] = bg;

    for (uint32_t y = 20; y < 120 && y < h; ++y)
        for (uint32_t x = 20; x < 320 && x < w; ++x)
            fb[y * stride + x] = fg;

    halt_forever();
    return 0;
}

