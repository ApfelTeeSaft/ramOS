/* heap.c - Simple heap allocator implementation */

#include "heap.h"
#include "memory.h"

#define HEAP_MAGIC 0xDEADBEEF

/* Heap block header */
typedef struct heap_block {
    uint32_t magic;
    uint32_t size;
    uint8_t is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = NULL;
static uint32_t heap_size = 0;

/* Initialize heap */
void heap_init(void) {
    /* Heap starts after kernel end (already set up by memory_init) */
    heap_start = (heap_block_t*)memory_get_heap_start();
    heap_size = 1024 * 1024; /* 1MB initial heap */
    
    /* Create first free block */
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = heap_size - sizeof(heap_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
}

/* Allocate memory */
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* Align to 4 bytes */
    size = (size + 3) & ~3;
    
    /* Find free block */
    heap_block_t* current = heap_start;
    
    while (current) {
        if (current->magic != HEAP_MAGIC) {
            /* Heap corruption */
            return NULL;
        }
        
        if (current->is_free && current->size >= size) {
            /* Found suitable block */
            
            /* Split block if remaining space is significant */
            if (current->size > size + sizeof(heap_block_t) + 16) {
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + 
                                          sizeof(heap_block_t) + size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        
        current = current->next;
    }
    
    /* No suitable block found - expand heap */
    return NULL;
}

/* Allocate aligned memory */
void* kmalloc_aligned(size_t size, uint32_t alignment) {
    /* Simple implementation - allocate extra and align */
    void* ptr = kmalloc(size + alignment);
    if (!ptr) return NULL;
    
    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    
    return (void*)aligned_addr;
}

/* Free memory */
void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    
    if (block->magic != HEAP_MAGIC) {
        /* Invalid pointer */
        return;
    }
    
    block->is_free = 1;
    
    /* Coalesce with next block if free */
    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
    }
    
    /* TODO: Coalesce with previous block */
}

/* Get heap statistics */
uint32_t heap_get_used(void) {
    uint32_t used = 0;
    heap_block_t* current = heap_start;
    
    while (current) {
        if (!current->is_free) {
            used += current->size + sizeof(heap_block_t);
        }
        current = current->next;
    }
    
    return used;
}

uint32_t heap_get_free(void) {
    uint32_t free = 0;
    heap_block_t* current = heap_start;
    
    while (current) {
        if (current->is_free) {
            free += current->size;
        }
        current = current->next;
    }
    
    return free;
}