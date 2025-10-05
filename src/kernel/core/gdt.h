/* gdt.h - Global Descriptor Table setup */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Initialize GDT */
void gdt_init(void);

#endif /* GDT_H */