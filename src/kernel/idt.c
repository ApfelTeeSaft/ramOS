/* idt.c - Interrupt Descriptor Table implementation */

#include "idt.h"

/* IDT entry structure */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

/* IDT pointer structure */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* IDT with 256 entries */
static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

/* External assembly function to load IDT */
extern void idt_flush(uint32_t);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    idt_pointer.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_pointer.base = (uint32_t)&idt;
    
    /* Clear IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    /* Load IDT */
    idt_flush((uint32_t)&idt_pointer);
}

/* Assembly stub to load IDT */
__asm__(
    ".global idt_flush\n"
    "idt_flush:\n"
    "   mov 4(%esp), %eax\n"
    "   lidt (%eax)\n"
    "   ret\n"
);