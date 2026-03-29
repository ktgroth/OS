#ifndef __CPU_IRQ
#define __CPU_IRQ

#include "../../../types.h"
#include "isr.h"

typedef void (*irq_handler_t)(interrupt_frame_t *frame);

void init_irq(void);
void irq_register_handler(uint8_t irq, irq_handler_t handler);
void irq_dispatch(interrupt_frame_t *frame);
void pic_send_eoi(uint8_t vector);

#endif

