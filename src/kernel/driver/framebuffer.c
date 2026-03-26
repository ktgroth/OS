
#include "../include/driver/framebuffer.h"
#include "../include/libc/math.h"
#include "../include/libc/stdlib.h"
#include "../include/libc/printf.h"

#define FB_INFO_BASE    0x7000ULL

typedef struct __attribute__((packed)) {
    uint8_t ready;
    uint8_t bpp;
    uint16_t reserved;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t addr_low;
    uint32_t addr_high;
} boot_fb_info_t;

static framebuffer_t g_fb;

extern void dbg_puts(const char *s);

uint8_t fb_init_from_bootinfo() {
    boot_fb_info_t *bi = (boot_fb_info_t *)FB_INFO_BASE;
    if (bi->ready != 1 || bi->width == 0 || bi->height == 0 || bi->pitch == 0) {
        g_fb.ready = 0;
        return 0;
    }

    uint64_t addr = ((uint64_t)bi->addr_high << 32) | bi->addr_low;
    g_fb.addr = (__volatile__ uint8_t *)addr;
    g_fb.width = bi->width;
    g_fb.height = bi->height;
    g_fb.pitch = bi->pitch;
    g_fb.bpp = bi->bpp;
    g_fb.ready = 1;

    return 1;
}

framebuffer_t *fb_get() {
    return &g_fb;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb.ready || x >= g_fb.width || y >= g_fb.height)
        return;

    uint32_t bytes_per_pixel = g_fb.bpp / 8;
    __volatile__ uint8_t *p = g_fb.addr + (uint64_t)y * g_fb.pitch + (uint64_t)x * bytes_per_pixel;

    if (bytes_per_pixel == 4) {
        *(__volatile__ uint32_t *)p = color;
    } else if (bytes_per_pixel == 3) {
        p[0] = (uint8_t)(color & 0xFF);
        p[1] = (uint8_t)((color >> 8) & 0xFF);
        p[2] = (uint8_t)((color >> 16) & 0xFF);
    }
}

void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    int32_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    for (;;) {
        fb_put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;

        int32_t e2 = err << 1;
        if (e2 >= -dy) {
            err -= dy;
            x1 = (uint32_t)((int32_t)x1 + sx);
        }

        if (e2 <= dx) {
            err += dx;
            y1 = (uint32_t)((int32_t)y1 + sy);
        }
    }
}

void fb_draw_circ(uint32_t x, uint32_t y, uint32_t r, uint32_t color) {
    int32_t t1 = r / 16;
    int32_t a = r;
    int32_t b = 0;

    while (a >= b) {
        fb_put_pixel(x + a, y + b, color);
        fb_put_pixel(x - a, y + b, color);
        fb_put_pixel(x + a, y - b, color);
        fb_put_pixel(x - a, y - b, color);
        fb_put_pixel(x + b, y + a, color);
        fb_put_pixel(x - b, y + a, color);
        fb_put_pixel(x + b, y - a, color);
        fb_put_pixel(x - b, y - a, color);

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
    if (!g_fb.ready)
        return;

    uint32_t x2 = x + w;
    uint32_t y2 = y + h;
    if (x2 > g_fb.width)
        x2 = g_fb.width;
    if (y2 > g_fb.height)
        y2 = g_fb.height;

    for (uint32_t yy = y; yy < y2; ++yy)
        for (uint32_t xx = x; xx < x2; ++xx)
            fb_put_pixel(xx, yy, color);
}

void fb_clear(uint32_t color) {
    if (!g_fb.ready)
        return;

    fb_fill_rect(0, 0, g_fb.width, g_fb.height, color);
}

