#ifndef __DISPLAY_FRAMEBUFFER
#define __DISPLAY_FRAMEBUFFER

#include "../../../boot_info.h"
#include "../../../types.h"

void init_fb(framebuffer_t fb);
void fb_set_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void fb_draw_circ(uint32_t x, uint32_t y, uint32_t r, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
framebuffer_t *fb_get(void);

#endif

