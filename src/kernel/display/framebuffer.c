
#include "../../boot_info.h"
#include "../include/display/framebuffer.h"


framebuffer_t g_fb;

static uint32_t format_color(uint32_t color) {
    uint8_t r = (color & 0xFF0000) >> 16;
    uint8_t g = (color & 0x00FF00) >> 8;
    uint8_t b = (color & 0x0000FF);

    if (g_fb.format == 1)
        return ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);
    else
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
}

void init_fb(framebuffer_t fb) {
    g_fb = fb;
}

void fb_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    color = format_color(color);

    uint32_t stride = g_fb.ppl;
    g_fb.base[y * stride + x] = color;
}

void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;

    for (;;) {
        fb_set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;

        int32_t e2 = 2 * err;
        if (e2 >= -dy) {
            err -= dy;
            x0 = (uint32_t)((int32_t)x0 + sx);
        } if (e2 <= dx) {
            err += dx;
            y0 = (uint32_t)((int32_t)y0 + sy);
        }
    }
}

void fb_draw_circ(uint32_t x, uint32_t y, uint32_t r, uint32_t color) {
    int32_t t1 = r / 16;
    int32_t a = r;
    int32_t b = 0;

    while (a >= b) {
        fb_set_pixel(x + a, y + b, color);
        fb_set_pixel(x - a, y + b, color);
        fb_set_pixel(x + a, y - b, color);
        fb_set_pixel(x - a, y - b, color);
        fb_set_pixel(x + b, y + a, color);
        fb_set_pixel(x - b, y + a, color);
        fb_set_pixel(x + b, y - a, color);
        fb_set_pixel(x - b, y - a, color);

        b += 1;
        t1 = t1 + b;
        int32_t t2 = t1 - a;
        if (t2 >= 0) {
            t1 = t2;
            a = a - 1;
        }
    }
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    uint32_t x1 = x + w;
    uint32_t y1 = y + h;
    if (x1 > g_fb.width)
        x1 = g_fb.width;
    if (y1 > g_fb.height)
        y1 = g_fb.height;

    for (uint32_t yy = y; yy < y1; ++yy)
        for (uint32_t xx = x; xx < x1; ++xx)
            fb_set_pixel(xx, yy, color);
}

framebuffer_t *fb_get(void) {
    return &g_fb;
}

