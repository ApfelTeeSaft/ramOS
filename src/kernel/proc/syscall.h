/* syscall.h - System call interface */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "../core/isr.h"

/* Initialize system call handler */
void syscall_init(void);

/* System call dispatcher */
int syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Assembly syscall stub */
extern void syscall_stub(void);

#endif /* SYSCALL_H */