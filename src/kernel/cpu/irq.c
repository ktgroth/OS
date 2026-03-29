
#include "../include/cpu/irq.h"
#include "../include/cpu/idt.h"
#include "../include/cpu/io.h"

#define IDT_INT_GATE    0x8E

#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC2_CMD        0xA0
#define PIC2_DATA       0xA1

#define PIC_EOI         0x20
#define ICW1_ICW4       0x01
#define ICW1_INIT       0x10
#define ICW4_8086       0x01


static irq_handler_t g_irq_handlers[16];

static void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();

    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();

    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();

    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(uint8_t vector) {
    if (vector >= 40)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16)
        g_irq_handlers[irq] = handler;
}

void init_irq(void) {
    uint32_t i;

    for (i = 0; i < 16; ++i)
        g_irq_handlers[i] = 0;

    pic_remap(32, 40);
    for (i = 32; i < 48; ++i)
        idt_set_gate((uint8_t)i, isr_stub_table[i], IDT_INT_GATE, 0);

    outb(PIC1_DATA, 0xFC);
    outb(PIC2_DATA, 0xFF);
}

void irq_dispatch(interrupt_frame_t *frame) {
    uint8_t vector = (uint8_t)frame->vector;
    uint8_t irq = (uint8_t)(vector - 32);

    if (irq < 16 && g_irq_handlers[irq] != 0)
        g_irq_handlers[irq](frame);

    pic_send_eoi(vector);
}
