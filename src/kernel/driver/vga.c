
#include "../include/cpu/ports.h"
#include "../include/driver/vga.h"

volatile vga_char *TEXT_AREA = (vga_char*) VGA_START;

static inline uint8_t vga_color(const uint8_t fg_color, const uint8_t bg_color){
    return (bg_color << 4) | (fg_color & 0x0F);
}

static inline void set_hw_cursor(uint16_t pos) {
    if (pos >= VGA_EXTENT) {
        pos = VGA_EXTENT - 1;
    }

    outb(CURSOR_PORT_COMMAND, 0x0F);
    outb(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));

    outb(CURSOR_PORT_COMMAND, 0x0E);
    outb(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

uint16_t get_cursor_pos() {
    uint16_t position = 0;

    outb(CURSOR_PORT_COMMAND, 0x0F);
    position |= inb(CURSOR_PORT_DATA);

    outb(CURSOR_PORT_COMMAND, 0x0E);
    position |= inb(CURSOR_PORT_DATA) << 8;

    return position;
}

void set_cursor_pos(uint8_t x, uint8_t y) {
    uint16_t pos = (uint16_t) x + ((uint16_t)VGA_WIDTH * (uint16_t)y);
    set_hw_cursor(pos);
}

void show_cursor() {
    uint8_t current;

    outb(CURSOR_PORT_COMMAND, 0x0A);
    current = inb(CURSOR_PORT_DATA);
    outb(CURSOR_PORT_DATA, current & 0xC0);

    outb(CURSOR_PORT_COMMAND, 0x0B);
    current = inb(CURSOR_PORT_DATA);
    outb(CURSOR_PORT_DATA, current & 0xE0);
}

void hide_cursor() {
    outb(CURSOR_PORT_COMMAND, 0x0A);
    outb(CURSOR_PORT_DATA, 0x20);
}

void scroll_line() {
    for(uint16_t row = 1; row < VGA_HEIGHT; row++) {
        for(uint16_t col = 0; col < VGA_WIDTH; col++) {
            uint16_t to_pos = col + ((uint16_t)(row - 1) * VGA_WIDTH);
            uint16_t from_pos = col + (row * VGA_WIDTH);
            TEXT_AREA[to_pos] = TEXT_AREA[from_pos];
        }
    }

    uint16_t last_row = VGA_HEIGHT - 1;
    uint8_t style = vga_color(COLOR_WHT, COLOR_BLK);
    for(uint16_t col = 0; col < VGA_WIDTH; col++) {
        uint16_t pos = col + (uint16_t)(last_row * VGA_WIDTH);
        TEXT_AREA[pos].character = ' ';
        TEXT_AREA[pos].style = style;
    }

    set_hw_cursor((uint16_t)(last_row * VGA_WIDTH));
}

void advance_cursor() {
    uint16_t pos = get_cursor_pos();
    if (pos + 1 >= VGA_EXTENT) {
        scroll_line();
        return;
    }

    set_hw_cursor(pos + 1);
}

void reverse_cursor() {
    unsigned short pos = get_cursor_pos();
    if (pos == 0)
        return;

    set_hw_cursor((uint16_t)(pos - 1));
}

void clearwin(uint8_t fg_color, uint8_t bg_color){
    const char space = ' ';
    uint8_t clear_color = vga_color(fg_color, bg_color);

    const vga_char clear_char = {
        .character = space,
        .style = clear_color
    };

    for(uint64_t i = 0; i < VGA_EXTENT; i++) {
        TEXT_AREA[i] = clear_char;
    }

    set_hw_cursor(0);
}

void putchar(const char character, const uint8_t fg_color, const uint8_t bg_color){
    uint16_t pos = get_cursor_pos();

    if (character == '\n') {
        uint16_t row = (uint16_t)(pos / VGA_WIDTH);
        ++row;

        if (row >= VGA_HEIGHT)
            scroll_line();
        else
            set_cursor_pos(0, (uint8_t)row);
    } else if (character == '\b') {
        reverse_cursor();
        pos = get_cursor_pos();
        TEXT_AREA[pos].character = ' ';
        TEXT_AREA[pos].style = vga_color(fg_color, bg_color);
    } else if (character == '\r') {
        uint16_t row = (uint16_t)(pos / VGA_WIDTH);
        set_cursor_pos(0, row);
    } else if (character == '\t') {
        for (uint8_t i = 0; i < 4; i++)
            putchar(' ', fg_color, bg_color);
    } else {
        TEXT_AREA[pos].character = character;
        TEXT_AREA[pos].style = vga_color(fg_color, bg_color);
        advance_cursor();
    }
}

void putstr(const char *string, const uint8_t fg_color, const uint8_t bg_color){
    while (*string != '\0')
        putchar(*string++, fg_color, bg_color);
}

