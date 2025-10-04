/* isr.h - Interrupt Service Routines */

#ifndef ISR_H
#define ISR_H

#include <stdint.h>

/* Registers pushed by ISR stub */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

/* Initialize ISRs */
void isr_init(void);

/* ISR handler function type */
typedef void (*isr_handler_t)(registers_t*);

/* Register an ISR handler */
void isr_register_handler(uint8_t n, isr_handler_t handler);

#endif /* ISR_H */