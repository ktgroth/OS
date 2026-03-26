
#include "../include/driver/gui.h"
#include "../include/driver/framebuffer.h"

#define CURSOR_W 24
#define CURSOR_H 24

static uint32_t under[CURSOR_W * CURSOR_H];
static int32_t cur_x = 0;
static int32_t cur_y = 0;
static uint8_t cursor_visible = 0;


static const uint8_t cursor_shape[CURSOR_H][CURSOR_W] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,2,2,2,3,1,1,1,1,1,3,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,2,3,3,2,3,1,3,3,3,3,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,2,3,1,3,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,3,1,3,1,3,2,3,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,1,3,0,3,3,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,1,3,0,0,3,1,3,2,3,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,3,0,0,0,0,3,3,2,2,1,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,3,1,3,3,1,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,3,1,1,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static uint32_t fb_read_pixel(uint32_t x, uint32_t y) {
    framebuffer_t *fb = fb_get();
    if (!fb->ready || x >= fb->width || y >= fb->height)
        return 0;

    uint32_t bpp = fb->bpp / 8;
    volatile uint8_t *p = fb->addr + (uint64_t)y * fb->pitch + (uint64_t)x * bpp;
    if (bpp == 4)
        return *(volatile uint32_t *)p;
    if (bpp == 3)
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    return 0;
}

static void cursor_save_bg(int32_t x, int32_t y) {
    framebuffer_t *fb = fb_get();
    for (uint32_t j = 0; j < CURSOR_H; ++j) {
        for (uint32_t i = 0; i < CURSOR_W; ++i) {
            int32_t px = x + (int32_t)i;
            int32_t py = y + (int32_t)j;
            uint32_t idx = j * CURSOR_W + i;
            if (px >= 0 && py >= 0 && (uint32_t)px < fb->width && (uint32_t)py < fb->height)
                under[idx] = fb_read_pixel((uint32_t)px, (uint32_t)py);
            else
                under[idx] = 0;
        }
    }
}

static void cursor_restore_bg(int32_t x, int32_t y) {
    framebuffer_t *fb = fb_get();
    for (uint32_t j = 0; j < CURSOR_H; ++j) {
        for (uint32_t i = 0; i < CURSOR_W; ++i) {
            int32_t px = x + (int32_t)i;
            int32_t py = y + (int32_t)j;
            if (px >= 0 && py >= 0 && (uint32_t)px < fb->width && (uint32_t)py < fb->height)
                fb_put_pixel((uint32_t)px, (uint32_t)py, under[j * CURSOR_W + i]);
        }
    }
}

static void cursor_draw(int32_t x, int32_t y) {
    framebuffer_t *fb = fb_get();
    for (uint32_t j = 0; j < CURSOR_H; ++j) {
        for (uint32_t i = 0; i < CURSOR_W; ++i) {
            int32_t px = x + (int32_t)i;
            int32_t py = y + (int32_t)j;
            if (px < 0 || py < 0 || (uint32_t)px >= fb->width || (uint32_t)py >= fb->height)
                continue;

            uint8_t v = cursor_shape[j][i];
            if (v == 1)
                fb_put_pixel((uint32_t)px, (uint32_t)py, 0x00FFFFFF);
            else if (v == 2)
                fb_put_pixel((uint32_t)px, (uint32_t)py, 0x00000000);
            else if (v == 3)
                fb_put_pixel((uint32_t)px, (uint32_t)py, 0x00404040);
        }
    }
}

void gui_cursor_init(void) {
    cursor_visible = 0;
    cur_x = 0;
    cur_y = 0;
}

void gui_cursor_move(int32_t x, int32_t y) {
    if (cursor_visible)
        cursor_restore_bg(cur_x, cur_y);

    cur_x = x;
    cur_y = y;
    cursor_save_bg(cur_x, cur_y);
    cursor_draw(cur_x, cur_y);
    cursor_visible = 1;
}

void gui_draw_desktop(void) {
    framebuffer_t *fb = fb_get();
    if (!fb->ready)
        return;

    fb_clear(0x00141414);
    // fb_fill_rect(0, 0, fb->width, 32, 0x0E1722);
    // fb_fill_rect(20, 60, 520, 340, 0xE6EDF3);
    // fb_fill_rect(20, 60, 520, 28, 0x2563EB);
    // fb_fill_rect(500, 66, 12, 12, 0xEF4444);
    // fb_fill_rect(484, 66, 12, 12, 0xF59E0B);
    // fb_fill_rect(468, 66, 12, 12, 0x22C55E);
    // fb_fill_rect(0, fb->height - 40, fb->width, 40, 0x0B1220);
}
