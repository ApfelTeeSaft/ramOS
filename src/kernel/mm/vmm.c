/* vmm.c - Virtual Memory Manager (Complete Implementation) */

#include "vmm.h"
#include "heap.h"
#include "../core/console.h"

#define PAGE_DIRECTORY_INDEX(x) ((x) >> 22)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xFFF)

/* Current page directory */
static uint32_t* current_page_directory = NULL;
static uint32_t* kernel_page_directory = NULL;

/* Get page table entry */
static uint32_t* vmm_get_page(uint32_t virtual_addr, int create, uint32_t** page_directory) {
    uint32_t* pd = page_directory ? *page_directory : current_page_directory;
    if (!pd) return NULL;
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    
    /* Check if page table exists */
    if (!(pd[pd_index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        /* Create new page table */
        uint32_t* page_table = (uint32_t*)kmalloc_aligned(4096, 4096);
        if (!page_table) return NULL;
        
        /* Clear page table */
        for (int i = 0; i < 1024; i++) {
            page_table[i] = 0;
        }
        
        /* Add to page directory */
        pd[pd_index] = ((uint32_t)page_table) | PAGE_PRESENT | PAGE_WRITE;
    }
    
    /* Get page table */
    uint32_t* page_table = (uint32_t*)PAGE_GET_PHYSICAL_ADDRESS(&pd[pd_index]);
    
    return &page_table[pt_index];
}

/* Initialize VMM */
void vmm_init(void) {
    kprintf("[VMM] Initializing Virtual Memory Manager...\n");
    
    /* Create kernel page directory */
    kernel_page_directory = vmm_create_page_directory();
    if (!kernel_page_directory) {
        kprintf("[VMM] Failed to create kernel page directory\n");
        return;
    }
    
    /* Identity map first 4MB (kernel space) */
    kprintf("[VMM] Identity mapping kernel space (0x00000000 - 0x00400000)...\n");
    for (uint32_t i = 0; i < 0x400000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_PRESENT | PAGE_WRITE);
    }
    
    /* Switch to new page directory */
    current_page_directory = kernel_page_directory;
    
    kprintf("[VMM] Virtual memory initialized\n");
}

/* Map virtual to physical address */
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t* page = vmm_get_page(virtual_addr, 1, &current_page_directory);
    if (!page) {
        kprintf("[VMM] Failed to map page %x -> %x\n", virtual_addr, physical_addr);
        return;
    }
    
    *page = (physical_addr & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;
    
    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

/* Unmap virtual address */
void vmm_unmap_page(uint32_t virtual_addr) {
    uint32_t* page = vmm_get_page(virtual_addr, 0, &current_page_directory);
    if (!page) return;
    
    *page = 0;
    
    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

/* Get physical address */
uint32_t vmm_get_physical(uint32_t virtual_addr) {
    uint32_t* page = vmm_get_page(virtual_addr, 0, &current_page_directory);
    if (!page || !(*page & PAGE_PRESENT)) {
        return 0;
    }
    
    return PAGE_GET_PHYSICAL_ADDRESS(page) | (virtual_addr & 0xFFF);
}

/* Create page directory */
uint32_t* vmm_create_page_directory(void) {
    /* Allocate page directory */
    uint32_t* page_dir = (uint32_t*)kmalloc_aligned(4096, 4096);
    
    if (!page_dir) return NULL;
    
    /* Clear page directory */
    for (int i = 0; i < 1024; i++) {
        page_dir[i] = 0;
    }
    
    return page_dir;
}

/* Switch page directory */
void vmm_switch_page_directory(uint32_t* page_directory) {
    if (!page_directory) return;
    
    current_page_directory = page_directory;
    
    /* Load CR3 register with physical address of page directory */
    __asm__ volatile("mov %0, %%cr3" :: "r"(page_directory));
}

/* Get current page directory */
uint32_t* vmm_get_page_directory(void) {
    return current_page_directory;
}

/* Clone page directory for fork */
uint32_t* vmm_clone_page_directory(uint32_t* src) {
    if (!src) return NULL;
    
    uint32_t* new_pd = vmm_create_page_directory();
    if (!new_pd) return NULL;
    
    /* Copy all page directory entries */
    for (int i = 0; i < 1024; i++) {
        if (!(src[i] & PAGE_PRESENT)) continue;
        
        /* Get source page table */
        uint32_t* src_pt = (uint32_t*)PAGE_GET_PHYSICAL_ADDRESS(&src[i]);
        
        /* Create new page table */
        uint32_t* new_pt = (uint32_t*)kmalloc_aligned(4096, 4096);
        if (!new_pt) continue;
        
        /* Copy page table entries */
        for (int j = 0; j < 1024; j++) {
            new_pt[j] = src_pt[j];
        }
        
        /* Add to new page directory */
        new_pd[i] = ((uint32_t)new_pt) | (src[i] & 0xFFF);
    }
    
    return new_pd;
}