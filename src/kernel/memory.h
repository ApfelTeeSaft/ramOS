/* memory.h - Memory management */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

/* Initialize memory subsystem with multiboot info */
void memory_init(uint32_t total_kb);

/* Simple bump allocator */
void* kmalloc(size_t size);

/* Get memory statistics */
uint32_t memory_get_total(void);
uint32_t memory_get_used(void);
uint32_t memory_get_free(void);

#endif /* MEMORY_H */