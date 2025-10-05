/* pit.h - Programmable Interval Timer */

#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* Initialize PIT to specified frequency */
void pit_init(uint32_t frequency);

#endif /* PIT_H */