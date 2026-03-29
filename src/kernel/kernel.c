
#include "../boot_info.h"

#include "include/cpu/gdt.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/cpu/irq.h"

#include "include/display/framebuffer.h"
#include "include/display/gui.h"

#include "include/drivers/keyboard.h"


static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("hlt");
}

int kmain(boot_info_t *bi) {
    if (bi == 0 || bi->fb.base == 0 || bi->fb.width == 0 || bi->fb.height == 0)
        halt_forever();

    init_gdt();
    init_idt();
    init_isr();
    init_irq();

    init_fb(bi->fb);
    init_gui();

    irq_register_handler(1, keyboard_callback);
    __asm__ __volatile__("sti");

    halt_forever();
    return 0;
}

