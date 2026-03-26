
#include "../include/cpu/ports.h"
#include "../include/cpu/isr.h"
#include "../include/driver/keyboard.h"
#include "../include/libc/printf.h"
#include "../include/libc/function.h"
#include "../include/libc/string.h"
#include "../include/libc/memory.h"
#include "../include/user_mode/process.h"
#include "../include/user_mode/scheduler.h"
#include "../include/kernel.h"


#define BACKSPACE       0x0E
#define ENTER           0x1C
#define LCTRL_PRESS     0x1D
#define LCTRL_RELEASE   0x9D
#define C_PRESS         0x2E

static char key_buffer[256];
static uint8_t ctrl_down = 0;

#define SC_MAX 57
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

static void keyboard_callback(registers_t *regs) {
    uint8_t scancode = inb(0x60);

    if (scancode == LCTRL_PRESS) {
        ctrl_down = 1;
        UNUSED(regs);
        return;
    }

    if (scancode == LCTRL_RELEASE) {
        ctrl_down = 0;
        UNUSED(regs);
        return;
    }

    if (ctrl_down && scancode == C_PRESS) {
        scheduler_cancel_current();
        puts("^C\n");
        puts("> ");
        UNUSED(regs);
        return;
    }

    if (scancode & 0x80) {
        UNUSED(regs);
        return;
    }

    if (current_process() != 0) {
        UNUSED(regs);
        return;
    }

    if (scancode > SC_MAX) {
        UNUSED(regs);
        return;
    }

    if (scancode == BACKSPACE) {
        if (key_buffer[0]) {
            backspace(key_buffer);
            putc('\b');
        }
    } else if (scancode == ENTER) {
        putc('\n');
        user_input(key_buffer);
        key_buffer[0] = '\0';
    } else {
        char letter = sc_ascii[(int32_t)scancode];
        char str[2] = { letter, '\0' };
        append(key_buffer, letter);
        puts(str);
    }

    UNUSED(regs);
}

void init_keyboard() {
    register_interrupt_handler(IRQ1, keyboard_callback);
}
