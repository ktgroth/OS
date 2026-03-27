
#include "../include/cpu/idt.h"
#include "../include/cpu/isr.h"
#include "../include/cpu/ports.h"
#include "../include/cpu/timer.h"
#include "../include/cpu/apic.h"
#include "../include/driver/vga.h"
#include "../include/driver/keyboard.h"
#include "../include/driver/mouse.h"
#include "../include/libc/string.h"
#include "../include/libc/printf.h"

isr_t interrupt_handlers[IDT_ENTRIES];

char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TTS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

static inline void pic_io_wait(void) {
    __asm__ __volatile__("outb %%al, $0x80" : : "a"(0));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ __volatile__(
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t v) {
    uint32_t lo = (uint32_t)v;
    uint32_t hi = (uint32_t)(v >> 32);
    __asm__ __volatile__(
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
    );
}

static void disable_lapic(void) {
    const uint32_t IA32_APIC_BASE_MSR = 0x1B;
    uint64_t v = rdmsr(IA32_APIC_BASE_MSR);
    v &= ~(1ULL << 11);
    wrmsr(IA32_APIC_BASE_MSR, v);
}

static void pic_remap(void) {
    outb(0x20, 0x11); pic_io_wait();
    outb(0xA0, 0x11); pic_io_wait();
    outb(0x21, 0x20); pic_io_wait();
    outb(0xA1, 0x28); pic_io_wait();
    outb(0x21, 0x04); pic_io_wait();
    outb(0xA1, 0x02); pic_io_wait();
    outb(0x21, 0x01); pic_io_wait();
    outb(0xA1, 0x01); pic_io_wait();
    
    outb(0x21, 0xFE); pic_io_wait();
    outb(0xA1, 0xFF); pic_io_wait();
}

void isr_install(void) {
    set_idt_gate(0, (uint64_t)isr0);
    set_idt_gate(1, (uint64_t)isr1);
    set_idt_gate(2, (uint64_t)isr2);
    set_idt_gate(3, (uint64_t)isr3);
    set_idt_gate(4, (uint64_t)isr4);
    set_idt_gate(5, (uint64_t)isr5);
    set_idt_gate(6, (uint64_t)isr6);
    set_idt_gate(7, (uint64_t)isr7);
    set_idt_gate(8, (uint64_t)isr8);
    set_idt_gate(9, (uint64_t)isr9);
    set_idt_gate(10, (uint64_t)isr10);
    set_idt_gate(11, (uint64_t)isr11);
    set_idt_gate(12, (uint64_t)isr12);
    set_idt_gate(13, (uint64_t)isr13);
    set_idt_gate(14, (uint64_t)isr14);
    set_idt_gate(15, (uint64_t)isr15);
    set_idt_gate(16, (uint64_t)isr16);
    set_idt_gate(17, (uint64_t)isr17);
    set_idt_gate(18, (uint64_t)isr18);
    set_idt_gate(19, (uint64_t)isr19);
    set_idt_gate(20, (uint64_t)isr20);
    set_idt_gate(21, (uint64_t)isr21);
    set_idt_gate(22, (uint64_t)isr22);
    set_idt_gate(23, (uint64_t)isr23);
    set_idt_gate(24, (uint64_t)isr24);
    set_idt_gate(25, (uint64_t)isr25);
    set_idt_gate(26, (uint64_t)isr26);
    set_idt_gate(27, (uint64_t)isr27);
    set_idt_gate(28, (uint64_t)isr28);
    set_idt_gate(29, (uint64_t)isr29);
    set_idt_gate(30, (uint64_t)isr30);
    set_idt_gate(31, (uint64_t)isr31);
    set_idt_gate_with_flags(128, (uint64_t)isr128, 0xEE);

    set_idt_gate(32, (uint64_t)irq0);
    set_idt_gate(33, (uint64_t)irq1);
    set_idt_gate(34, (uint64_t)irq2);
    set_idt_gate(35, (uint64_t)irq3);
    set_idt_gate(36, (uint64_t)irq4);
    set_idt_gate(37, (uint64_t)irq5);
    set_idt_gate(38, (uint64_t)irq6);
    set_idt_gate(39, (uint64_t)irq7);
    set_idt_gate(40, (uint64_t)irq8);
    set_idt_gate(41, (uint64_t)irq9);
    set_idt_gate(42, (uint64_t)irq10);
    set_idt_gate(43, (uint64_t)irq11);
    set_idt_gate(44, (uint64_t)irq12);
    set_idt_gate(45, (uint64_t)irq13);
    set_idt_gate(46, (uint64_t)irq14);
    set_idt_gate(47, (uint64_t)irq15);

    set_idt();
}

void irq_install() {
    pic_remap();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    apic_enable_lapic();
    ioapic_route_irq0_to_vector32();
    ioapic_route_irq1_to_vector33();
    ioapic_route_irq12_to_vector44();

    init_timer(PIT_HZ);
    init_keyboard();
    init_mouse();
    __asm__ __volatile__("sti");
}

static inline uint64_t read_cr2(void) {
    uint64_t cr2;
    __asm__ __volatile__(
        "mov %%cr2, %0"
        : "=r"(cr2)
    );

    return cr2;
}

void isr_handler(registers_t *r) {
    if (interrupt_handlers[r->irq_number]) {
        interrupt_handlers[r->irq_number](r);
        return;
    }

    if (r->irq_number < 32) {
        printf("Exception: %s\n", exception_messages[r->irq_number]);
        if (r->irq_number == 14) {
            uint64_t cr2 = read_cr2();
            printf("PF: cr2=%lx err=%lx\n", cr2, r->error_code);
        }

        printf("ISR=%lu CS=%lx RFL=%lx RSP=%lx SS=%lx\n",
                r->irq_number, r->cs, r->eflags, r->rsp, r->ss);
        printf("At RIP=%lx\n", r->rip);

        for (;;)
            __asm__ __volatile__("hlt");
    }
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void irq_handler(registers_t *r) {
    if (r->irq_number >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);

    apic_eoi();
    if (interrupt_handlers[r->irq_number]) {
        isr_t handler = interrupt_handlers[r->irq_number];
        handler(r);
    }
}

void dbg_pre_iret(uint64_t *sp) {
    printf("PRE-IRET rip=%lx cs=%lx rfl=%lx\n", sp[0], sp[1], sp[2]);
}

