
#ifndef __DRIVER_VGA_TEXT
#define __DRIVER_VGA_TEXT

#include "../libc/types.h"

#define VGA_START 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_EXTENT 80 * 25

#define COLOR_BLK 0
#define COLOR_BLU 1
#define COLOR_GRN 2
#define COLOR_CYN 3
#define COLOR_RED 4
#define COLOR_PRP 5
#define COLOR_BRN 6
#define COLOR_GRY 7
#define COLOR_DGY 8
#define COLOR_LBU 9
#define COLOR_LGR 10
#define COLOR_LCY 11
#define COLOR_LRD 12
#define COLOR_LPP 13
#define COLOR_YEL 14
#define COLOR_WHT 15

#define CURSOR_PORT_COMMAND (uint16_t) 0x3D4
#define CURSOR_PORT_DATA    (uint16_t) 0x3D5

typedef struct __attribute__((packed)) {
    char character;
    char style;
} vga_char;

void clearwin(const uint8_t fg_color, const uint8_t bg_color);
void putchar(const char character, const uint8_t fg_color, const uint8_t bg_color);
void putstr(const char *string, const uint8_t fg_color, const uint8_t bg_color);

uint16_t get_cursor_pos();
void show_cursor();
void hide_cursor();
void advance_cursor();
void reverse_cursor();
void set_cursor_pos(uint8_t x, uint8_t y);

#endif
