
#include "../include/cpu/io.h"
#include "../include/display/framebuffer.h"
#include "../include/drivers/keyboard.h"

#define BACKSPACE   0x0E
#define ENTER       0x1C
#define LCTRL_P     0x1D
#define LCTRL_R     0x9D
#define C_P         0x2E

#define SC_MAX      57


static char key_buffer[256];
static char line_ready[256];
static volatile uint8_t line_pending = 0;
static uint8_t ctrl_down = 0;

const char *sc_name[] = {
    "ERROR", "ESC", "1", "2", "3", "4", "5", "6",
    "7", "8", "9", "0", "-", "+", "BACKSPACE", "TAB",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[",
    "]", "ENTER", "LCTRL", "A", "S", "D", "F", "G", "H", "J",
    "K", "L", ";", "\'", "`", "LSHIFT", "\\", "Z", "X", "C", "V",
    "B", "N", "M", ",", ".", "/", "RSHIFT", "KEYPAD *", "LALT", "SPACEBAR" 
};
const char sc_ascii[] = {
    '?', '?', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '+', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',
    'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F',
    'G', 'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '
};


void keyboard_callback(interrupt_frame_t *frame) {
    (void)frame;
    uint8_t sc = inb(0x60);


}
