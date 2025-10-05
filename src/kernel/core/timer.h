/* timer.h - System timer */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Initialize timer */
void timer_init(void);

/* Get tick count */
uint32_t timer_get_ticks(void);

/* Get uptime in milliseconds */
uint32_t timer_get_uptime_ms(void);

#endif /* TIMER_H */