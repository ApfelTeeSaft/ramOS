/* irq.h - Hardware interrupt requests */

#ifndef IRQ_H
#define IRQ_H

#include "isr.h"

/* Initialize IRQs */
void irq_init(void);

/* Register IRQ handler */
void irq_register_handler(uint8_t irq, isr_handler_t handler);

#endif /* IRQ_H */