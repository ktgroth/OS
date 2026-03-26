
#include "../include/cpu/ports.h"
#include "../include/cpu/isr.h"
#include "../include/driver/mouse.h"
#include "../include/driver/gui.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

#define PS2_STATUS_OBF  (1 << 0)
#define PS2_STATUS_IBF  (1 << 1)
#define PS2_STATUS_AUX  (1 << 5)

static mouse_state_t g_mouse = { 0 };
static uint8_t packet[3];
static uint8_t packet_index = 0;
static uint32_t max_x = 1024;
static uint32_t max_y = 768;

static void ps2_wait_write(void) {
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_IBF);
}

static uint8_t ps2_wait_read(void) {
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF));
    return inb(PS2_DATA_PORT);
}

static void ps2_write_cmd(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_CMD_PORT, cmd);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_write();
    outb(PS2_DATA_PORT, data);
}

static void mouse_write(uint8_t value) {
    ps2_write_cmd(0xD4);
    ps2_write_data(value);
}

static uint8_t mouse_read(void) {
    return ps2_wait_read();
}

static void mouse_callback(registers_t *regs) {
    (void)regs;

    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & PS2_STATUS_OBF) || !(status & PS2_STATUS_AUX)) 
        return;

    uint8_t data = inb(PS2_DATA_PORT);
    packet[packet_index++] = data;

    if (packet_index < 3)
        return;

    packet_index = 0;
    if ((packet[0] & 0x08) == 0)
        return;

    int8_t dx = (int8_t)packet[1];
    int8_t dy = (int8_t)packet[2];

    g_mouse.dx = dx;
    g_mouse.dy = dy;

    g_mouse.x += dx;
    g_mouse.y -= dy;

    if (g_mouse.x < 0)
        g_mouse.x = 0;
    if (g_mouse.y < 0)
        g_mouse.y = 0;

    if ((uint32_t)g_mouse.x >= max_x)
        g_mouse.x = (int32_t)(max_x - 1);
    if ((uint32_t)g_mouse.y >= max_y)
        g_mouse.y = (int32_t)(max_y - 1);

    g_mouse.left = packet[0] & 0x01 ? 1 : 0;
    g_mouse.right = packet[0] & 0x02 ? 1 : 0;
    g_mouse.middle = packet[0] & 0x04 ? 1 : 0;
    g_mouse.changed = 1;

    gui_cursor_move(g_mouse.x, g_mouse.y);
}

void init_mouse(void) {
    register_interrupt_handler(IRQ12, mouse_callback);

    ps2_write_cmd(0xA8);
    ps2_write_cmd(0x20);
    uint8_t config = ps2_wait_read();
    config |= (1 << 1);
    config &= ~(1 << 5);

    ps2_write_cmd(0x60);
    ps2_write_data(config);

    mouse_write(0xF6);
    (void)mouse_read();

    mouse_write(0xF4);
    (void)mouse_read();
}

void mouse_set_bounds(uint32_t width, uint32_t height) {
    if (width > 0)
        max_x = width;
    if (height > 0)
        max_y = height;
    
    if ((uint32_t)g_mouse.x >= max_x)
        g_mouse.x = (int32_t)(max_x - 1);
    if ((uint32_t)g_mouse.y >= max_y)
        g_mouse.y = (int32_t)(max_y - 1);
}

void mouse_get_state(mouse_state_t *out) {
    if (!out)
        return;

    *out = g_mouse;
    g_mouse.changed = 0;
}

