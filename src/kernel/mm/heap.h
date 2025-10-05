/* heap.h - Kernel heap allocator */

#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

/* Initialize heap */
void heap_init(void);

/* Allocate memory from heap */
void* kmalloc(size_t size);

/* Allocate aligned memory */
void* kmalloc_aligned(size_t size, uint32_t alignment);

/* Free memory */
void kfree(void* ptr);

/* Get heap statistics */
uint32_t heap_get_used(void);
uint32_t heap_get_free(void);

#endif /* HEAP_H */