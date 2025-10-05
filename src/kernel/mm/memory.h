/* memory.h - Memory management */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

/* Initialize memory subsystem with multiboot info */
void memory_init(uint32_t total_kb);

/* Simple bump allocator (for early allocation) */
void* kmalloc_early(size_t size);

/* Get memory statistics */
uint32_t memory_get_total(void);
uint32_t memory_get_used(void);
uint32_t memory_get_free(void);

/* Get heap start address */
uint32_t memory_get_heap_start(void);

#endif /* MEMORY_H */