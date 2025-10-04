/* isr.c - Interrupt Service Routines implementation */

#include "isr.h"
#include "idt.h"
#include "console.h"

/* ISR handler array */
static isr_handler_t isr_handlers[256];

/* Exception messages */
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

/* External ISR stubs defined in assembly */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

void isr_init(void) {
    /* Set up ISR gates for CPU exceptions (0-31) */
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    
    /* Clear handlers */
    for (int i = 0; i < 256; i++) {
        isr_handlers[i] = 0;
    }
}

void isr_register_handler(uint8_t n, isr_handler_t handler) {
    isr_handlers[n] = handler;
}

/* Common ISR handler called from assembly stubs */
void isr_handler(registers_t* regs) {
    /* Call custom handler if registered */
    if (isr_handlers[regs->int_no] != 0) {
        isr_handler_t handler = isr_handlers[regs->int_no];
        handler(regs);
    } else {
        /* Unhandled exception */
        if (regs->int_no < 32) {
            kprintf("\n!!! EXCEPTION: %s !!!\n", exception_messages[regs->int_no]);
            kprintf("Error code: %u\n", regs->err_code);
            kprintf("EIP: %x\n", regs->eip);
            kprintf("System halted.\n");
            
            /* Halt */
            __asm__ volatile("cli; hlt");
        }
    }
}

/* Assembly ISR stubs - these push interrupt number and call isr_handler */
__asm__(
    ".macro ISR_NOERRCODE num\n"
    ".global isr\\num\n"
    "isr\\num:\n"
    "   cli\n"
    "   push $0\n"
    "   push $\\num\n"
    "   jmp isr_common_stub\n"
    ".endm\n"
    "\n"
    ".macro ISR_ERRCODE num\n"
    ".global isr\\num\n"
    "isr\\num:\n"
    "   cli\n"
    "   push $\\num\n"
    "   jmp isr_common_stub\n"
    ".endm\n"
    "\n"
    "ISR_NOERRCODE 0\n"
    "ISR_NOERRCODE 1\n"
    "ISR_NOERRCODE 2\n"
    "ISR_NOERRCODE 3\n"
    "ISR_NOERRCODE 4\n"
    "ISR_NOERRCODE 5\n"
    "ISR_NOERRCODE 6\n"
    "ISR_NOERRCODE 7\n"
    "ISR_ERRCODE 8\n"
    "ISR_NOERRCODE 9\n"
    "ISR_ERRCODE 10\n"
    "ISR_ERRCODE 11\n"
    "ISR_ERRCODE 12\n"
    "ISR_ERRCODE 13\n"
    "ISR_ERRCODE 14\n"
    "ISR_NOERRCODE 15\n"
    "ISR_NOERRCODE 16\n"
    "ISR_ERRCODE 17\n"
    "ISR_NOERRCODE 18\n"
    "ISR_NOERRCODE 19\n"
    "ISR_NOERRCODE 20\n"
    "ISR_NOERRCODE 21\n"
    "ISR_NOERRCODE 22\n"
    "ISR_NOERRCODE 23\n"
    "ISR_NOERRCODE 24\n"
    "ISR_NOERRCODE 25\n"
    "ISR_NOERRCODE 26\n"
    "ISR_NOERRCODE 27\n"
    "ISR_NOERRCODE 28\n"
    "ISR_NOERRCODE 29\n"
    "ISR_ERRCODE 30\n"
    "ISR_NOERRCODE 31\n"
    "\n"
    "isr_common_stub:\n"
    "   pusha\n"
    "   push %ds\n"
    "   mov $0x10, %ax\n"
    "   mov %ax, %ds\n"
    "   mov %ax, %es\n"
    "   mov %ax, %fs\n"
    "   mov %ax, %gs\n"
    "   push %esp\n"
    "   call isr_handler\n"
    "   add $4, %esp\n"
    "   pop %ds\n"
    "   popa\n"
    "   add $8, %esp\n"
    "   iret\n"
);