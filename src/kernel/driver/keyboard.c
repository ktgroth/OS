
#include "../include/cpu/ports.h"
#include "../include/cpu/isr.h"
#include "../include/driver/keyboard.h"
#include "../include/libc/printf.h"
#include "../include/libc/function.h"
#include "../include/libc/stdlib.h"
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
static char line_ready[256];
static volatile uint8_t line_pending = 0;
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

static inline uint64_t irq_save_disable(void) {
    uint64_t flags;
    __asm__ __volatile__("pushfq; popq %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint64_t flags) {
    if (flags & (1ULL << 9))
        __asm__ __volatile__("sti" ::: "memory");
}

static void append_char_bounded(char *buf, uint64_t cap, char c) {
    uint64_t n = 0;
    while (n + 1 < cap && buf[n])
        ++n;
    if (n + 1 < cap) {
        buf[n] = c;
        buf[n + 1] = '\0';
    }
}

static void keyboard_callback(registers_t *regs) {
    uint8_t scancode = inb(0x60);

    if (scancode == LCTRL_PRESS) {
        ctrl_down = 1;
        return;
    } if (scancode == LCTRL_RELEASE) {
        ctrl_down = 0;
        return;
    }

    if (scancode & 0x80)
        return;

    if (ctrl_down && scancode == C_PRESS) {
        scheduler_cancel_current();
        if ((regs->cs & 0x03) == 0x03)
            scheduler_on_tick(regs);
        return;
    }

    UNUSED(regs);
    if (line_pending)
        return;

    if (scancode == BACKSPACE) {
        if (key_buffer[0]) {
            backspace(key_buffer);
            putc('\b');
        }
        return;
    } 

    if (scancode == ENTER) {
        memcpy((uint8_t *)line_ready, (uint8_t *)key_buffer, strlen(key_buffer) + 1);
        key_buffer[0] = '\0';
        line_pending = 1;
        putc('\n');
        return;
    }

    if (scancode <= SC_MAX) {
        char c = sc_ascii[(int32_t)scancode];
        if (c != '?') {
            append_char_bounded(key_buffer, sizeof(key_buffer), c);
            putc(c);
        }
    }
}

void init_keyboard() {
    register_interrupt_handler(IRQ1, keyboard_callback);
}

uint8_t keyboard_try_get_line(char *out, uint64_t out_sz) {
    if (!out || out_sz == 0)
        return 0;

    uint64_t f = irq_save_disable();

    if (!line_pending) {
        irq_restore(f);
        return 0;
    }

    uint64_t n = strlen(line_ready);
    if (n >= out_sz)
        n = out_sz - 1;

    memcpy((uint8_t *)out, (uint8_t *)line_ready, n);
    out[n] = '\0';
    line_pending = 0;

    irq_restore(f);
    return 1;
}

