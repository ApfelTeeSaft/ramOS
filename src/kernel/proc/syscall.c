/* syscall.c - System call interrupt handler */

#include "syscall.h"
#include "../core/idt.h"
#include "../core/isr.h"

/* External syscall dispatcher from syscall_table.c */
extern int syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* System call interrupt handler - NOT static so assembly can call it */
void syscall_handler(registers_t* regs) {
    /* Extract syscall number and arguments from registers */
    uint32_t syscall_num = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    
    /* Dispatch to appropriate handler */
    int ret = syscall_dispatch(syscall_num, arg1, arg2, arg3);
    
    /* Return value in EAX */
    regs->eax = ret;
}

/* Initialize system call interface */
void syscall_init(void) {
    /* Register INT 0x80 handler */
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0x8E);
}

/* Assembly stub for system calls */
__asm__(
    ".global syscall_stub\n"
    "syscall_stub:\n"
    "   pusha\n"
    "   push %ds\n"
    "   mov $0x10, %ax\n"
    "   mov %ax, %ds\n"
    "   mov %ax, %es\n"
    "   mov %ax, %fs\n"
    "   mov %ax, %gs\n"
    "   push %esp\n"
    "   call syscall_handler\n"
    "   add $4, %esp\n"
    "   pop %ds\n"
    "   popa\n"
    "   iret\n"
);