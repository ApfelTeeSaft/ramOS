/* memory.c - Simple memory management with bump allocator */

#include "memory.h"

/* External symbols from linker script */
extern uint32_t kernel_end;

/* Memory tracking */
static uint32_t total_memory = 0;      /* Total memory in bytes */
static uint32_t heap_start = 0;        /* Start of heap */
static uint32_t heap_current = 0;      /* Current heap position */

void memory_init(uint32_t total_kb) {
    total_memory = total_kb * 1024;
    
    /* Align heap start to 4KB boundary after kernel */
    heap_start = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFF;
    heap_current = heap_start;
}

uint32_t memory_get_total(void) {
    return total_memory;
}

uint32_t memory_get_used(void) {
    return heap_current - heap_start;
}

uint32_t memory_get_free(void) {
    uint32_t used = heap_current - heap_start;
    if (total_memory > used) {
        return total_memory - used;
    }
    return 0;
}

uint32_t memory_get_heap_start(void) {
    return heap_start;
}