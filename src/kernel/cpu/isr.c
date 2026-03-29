
#include "../include/cpu/isr.h"
#include "../include/cpu/idt.h"
#include "../include/cpu/irq.h"

#define IDT_INT_GATE    0x8E


static isr_handler_t g_handlers[256];


static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("cli; hlt");
}

void isr_register_handler(uint8_t vector, isr_handler_t handler) {
    g_handlers[vector] = handler;
}

void init_isr(void) {
    uint32_t i;
    for (i = 0; i < 256; ++i)
        g_handlers[i] = 0;

    for (i = 0; i < 32; ++i)
        idt_set_gate((uint8_t)i, isr_stub_table[i], IDT_INT_GATE, 0);
}

void isr_dispatch(interrupt_frame_t *frame) {
    uint8_t vector = (uint8_t)frame->vector;

    if (vector < 32) {
        if (g_handlers[vector] != 0) {
            g_handlers[vector](frame);
            return;
        }
        halt_forever();
    }

    if (vector < 48) {
        irq_dispatch(frame);
        return;
    }

    if (g_handlers[vector] != 0)
        g_handlers[vector](frame);
}
