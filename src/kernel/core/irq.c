/* irq.c - Hardware interrupt request handling
 *
 * Remaps the PIC (Programmable Interrupt Controller) and handles IRQs.
 */

#include "irq.h"
#include "idt.h"
#include "isr.h"

/* PIC I/O ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

/* PIC commands */
#define PIC_EOI 0x20

/* IRQ handlers */
static isr_handler_t irq_handlers[16];

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* External IRQ stubs */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Remap PIC to avoid conflicts with CPU exceptions */
static void pic_remap(void) {
    uint8_t mask1, mask2;
    
    /* Save masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    
    /* Start initialization sequence */
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    
    /* Set vector offsets (IRQ 0-7 -> INT 32-39, IRQ 8-15 -> INT 40-47) */
    outb(PIC1_DATA, 32);
    outb(PIC2_DATA, 40);
    
    /* Configure cascading */
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    
    /* 8086 mode */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    /* Restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void irq_init(void) {
    /* Remap PIC */
    pic_remap();
    
    /* Install IRQ handlers in IDT (32-47) */
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    
    /* Clear handlers */
    for (int i = 0; i < 16; i++) {
        irq_handlers[i] = 0;
    }
    
    /* Enable interrupts */
    __asm__ volatile("sti");
}

void irq_register_handler(uint8_t irq, isr_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

/* Common IRQ handler */
void irq_handler(registers_t* regs) {
    /* Call custom handler if registered */
    uint8_t irq = regs->int_no - 32;
    if (irq_handlers[irq] != 0) {
        isr_handler_t handler = irq_handlers[irq];
        handler(regs);
    }
    
    /* Send EOI to PIC */
    if (regs->int_no >= 40) {
        /* Send EOI to slave PIC */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    /* Always send EOI to master PIC */
    outb(PIC1_COMMAND, PIC_EOI);
}

/* Assembly IRQ stubs */
__asm__(
    ".macro IRQ num, int_no\n"
    ".global irq\\num\n"
    "irq\\num:\n"
    "   cli\n"
    "   push $0\n"
    "   push $\\int_no\n"
    "   jmp irq_common_stub\n"
    ".endm\n"
    "\n"
    "IRQ 0, 32\n"
    "IRQ 1, 33\n"
    "IRQ 2, 34\n"
    "IRQ 3, 35\n"
    "IRQ 4, 36\n"
    "IRQ 5, 37\n"
    "IRQ 6, 38\n"
    "IRQ 7, 39\n"
    "IRQ 8, 40\n"
    "IRQ 9, 41\n"
    "IRQ 10, 42\n"
    "IRQ 11, 43\n"
    "IRQ 12, 44\n"
    "IRQ 13, 45\n"
    "IRQ 14, 46\n"
    "IRQ 15, 47\n"
    "\n"
    "irq_common_stub:\n"
    "   pusha\n"
    "   push %ds\n"
    "   mov $0x10, %ax\n"
    "   mov %ax, %ds\n"
    "   mov %ax, %es\n"
    "   mov %ax, %fs\n"
    "   mov %ax, %gs\n"
    "   push %esp\n"
    "   call irq_handler\n"
    "   add $4, %esp\n"
    "   pop %ds\n"
    "   popa\n"
    "   add $8, %esp\n"
    "   iret\n"
);