/* idt.h - Interrupt Descriptor Table */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Initialize IDT */
void idt_init(void);

/* Set an IDT gate */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif /* IDT_H */