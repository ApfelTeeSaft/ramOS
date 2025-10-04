/* gdt.c - Global Descriptor Table implementation
 * 
 * Sets up a flat memory model with kernel code and data segments.
 */

#include "gdt.h"

/* GDT entry structure */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/* GDT pointer structure */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* GDT with 5 entries: null, kernel code, kernel data, user code, user data */
static struct gdt_entry gdt[5];
static struct gdt_ptr gdt_pointer;

/* External assembly function to load GDT */
extern void gdt_flush(uint32_t);

/* Set a GDT entry */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_init(void) {
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_pointer.base = (uint32_t)&gdt;
    
    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Kernel code segment: base=0, limit=4GB, access=0x9A, granularity=0xCF */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    /* Kernel data segment: base=0, limit=4GB, access=0x92, granularity=0xCF */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    /* User code segment: base=0, limit=4GB, access=0xFA, granularity=0xCF */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    /* User data segment: base=0, limit=4GB, access=0xF2, granularity=0xCF */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    /* Load the new GDT */
    gdt_flush((uint32_t)&gdt_pointer);
}

/* Assembly stub to load GDT */
__asm__(
    ".global gdt_flush\n"
    "gdt_flush:\n"
    "   mov 4(%esp), %eax\n"
    "   lgdt (%eax)\n"
    "   mov $0x10, %ax\n"
    "   mov %ax, %ds\n"
    "   mov %ax, %es\n"
    "   mov %ax, %fs\n"
    "   mov %ax, %gs\n"
    "   mov %ax, %ss\n"
    "   jmp $0x08, $.flush\n"
    ".flush:\n"
    "   ret\n"
);