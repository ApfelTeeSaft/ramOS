/* timer.c - System timer implementation using PIT */

#include "timer.h"
#include "pit.h"
#include "irq.h"
#include "isr.h"

/* Timer frequency (100 Hz = 10ms per tick) */
#define TIMER_FREQ 100

/* Tick counter */
static volatile uint32_t tick_count = 0;

/* Timer IRQ handler */
static void timer_handler(registers_t* regs) {
    (void)regs;
    tick_count++;
}

void timer_init(void) {
    /* Register timer IRQ handler (IRQ 0) */
    irq_register_handler(0, timer_handler);
    
    /* Initialize PIT */
    pit_init(TIMER_FREQ);
}

uint32_t timer_get_ticks(void) {
    return tick_count;
}

uint32_t timer_get_uptime_ms(void) {
    /* Each tick is 10ms at 100Hz */
    return tick_count * 10;
}