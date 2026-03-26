#ifndef __DRIVER_FRAMEBUFFER_H
#define __DRIVER_FRAMEBUFFER_H

#include "../libc/types.h"

typedef struct {
    __volatile__ uint8_t *addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint8_t ready;
} framebuffer_t;

uint8_t fb_init_from_bootinfo();
framebuffer_t *fb_get();

void fb_clear(uint32_t color);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void fb_draw_circ(uint32_t x, uint32_t y, uint32_t r, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

#endif

