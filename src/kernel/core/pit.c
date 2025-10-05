/* pit.c - Programmable Interval Timer implementation
 *
 * Configures the PIT to generate timer interrupts at a specific frequency.
 */

#include "pit.h"

/* PIT I/O ports */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43

/* PIT base frequency (1.193182 MHz) */
#define PIT_BASE_FREQ 1193182

/* Port I/O */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void pit_init(uint32_t frequency) {
    /* Calculate divisor */
    uint32_t divisor = PIT_BASE_FREQ / frequency;
    
    /* Send command byte: channel 0, lobyte/hibyte, rate generator */
    outb(PIT_COMMAND, 0x36);
    
    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}