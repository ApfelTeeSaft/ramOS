/* vmm.c - Virtual Memory Manager (Fixed Implementation) */

#include "vmm.h"
#include "heap.h"
#include "memory.h"
#include "../core/console.h"

#define PAGE_DIRECTORY_INDEX(x) ((x) >> 22)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xFFF)

/* Page frame allocator - simple bitmap */
#define MAX_FRAMES 32768  /* 128MB / 4KB */
static uint32_t frame_bitmap[MAX_FRAMES / 32];
static uint32_t next_free_frame = 0;

/* Current page directory */
static uint32_t* current_page_directory = NULL;
static uint32_t* kernel_page_directory = NULL;

/* Allocate a physical frame */
static uint32_t alloc_frame(void) {
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        uint32_t frame = (next_free_frame + i) % MAX_FRAMES;
        uint32_t byte = frame / 32;
        uint32_t bit = frame % 32;
        
        if (!(frame_bitmap[byte] & (1 << bit))) {
            frame_bitmap[byte] |= (1 << bit);
            next_free_frame = (frame + 1) % MAX_FRAMES;
            return frame * PAGE_SIZE;
        }
    }
    
    return 0;  /* Out of memory */
}

/* Free a physical frame */
static void free_frame(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    if (frame >= MAX_FRAMES) return;
    
    uint32_t byte = frame / 32;
    uint32_t bit = frame % 32;
    frame_bitmap[byte] &= ~(1 << bit);
}

/* Get page table entry - fixed version */
static uint32_t* vmm_get_page(uint32_t virtual_addr, int create, uint32_t** page_directory) {
    uint32_t* pd = page_directory ? *page_directory : current_page_directory;
    if (!pd) return NULL;
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    
    /* Check if page table exists */
    if (!(pd[pd_index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        /* Allocate physical frame for page table */
        uint32_t pt_phys = alloc_frame();
        if (!pt_phys) {
            kprintf("[VMM] Failed to allocate frame for page table\n");
            return NULL;
        }
        
        /* Clear page table (use physical address directly during early boot) */
        uint32_t* page_table = (uint32_t*)pt_phys;
        for (int i = 0; i < 1024; i++) {
            page_table[i] = 0;
        }
        
        /* Add to page directory */
        pd[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    
    /* Get page table */
    uint32_t* page_table = (uint32_t*)PAGE_GET_PHYSICAL_ADDRESS(&pd[pd_index]);
    
    return &page_table[pt_index];
}

/* Initialize VMM */
void vmm_init(void) {
    kprintf("[VMM] Initializing Virtual Memory Manager...\n");
    
    /* Clear frame bitmap */
    for (int i = 0; i < MAX_FRAMES / 32; i++) {
        frame_bitmap[i] = 0;
    }
    
    /* Mark first 1MB + kernel as used */
    uint32_t kernel_frames = 0x400000 / PAGE_SIZE;  /* 4MB for kernel */
    for (uint32_t i = 0; i < kernel_frames; i++) {
        uint32_t byte = i / 32;
        uint32_t bit = i % 32;
        frame_bitmap[byte] |= (1 << bit);
    }
    next_free_frame = kernel_frames;
    
    /* Allocate kernel page directory */
    uint32_t pd_phys = alloc_frame();
    if (!pd_phys) {
        kprintf("[VMM] Failed to allocate kernel page directory\n");
        return;
    }
    
    kernel_page_directory = (uint32_t*)pd_phys;
    
    /* Clear page directory */
    for (int i = 0; i < 1024; i++) {
        kernel_page_directory[i] = 0;
    }
    
    current_page_directory = kernel_page_directory;
    
    /* Identity map first 8MB (kernel space) */
    kprintf("[VMM] Identity mapping kernel space (0x00000000 - 0x00800000)...\n");
    for (uint32_t i = 0; i < 0x800000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_PRESENT | PAGE_WRITE);
    }
    
    /* Enable paging */
    kprintf("[VMM] Enabling paging...\n");
    __asm__ volatile(
        "mov %0, %%cr3\n"  /* Load page directory */
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"  /* Set PG bit */
        "mov %%eax, %%cr0\n"
        :: "r"(pd_phys)
        : "eax"
    );
    
    kprintf("[VMM] Virtual memory initialized\n");
}

/* Map virtual to physical address */
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t* page = vmm_get_page(virtual_addr, 1, &current_page_directory);
    if (!page) {
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
    
    /* Free the physical frame */
    uint32_t phys = PAGE_GET_PHYSICAL_ADDRESS(page);
    if (phys) {
        free_frame(phys);
    }
    
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
    /* Allocate physical frame for page directory */
    uint32_t pd_phys = alloc_frame();
    if (!pd_phys) return NULL;
    
    uint32_t* page_dir = (uint32_t*)pd_phys;
    
    /* Clear page directory */
    for (int i = 0; i < 1024; i++) {
        page_dir[i] = 0;
    }
    
    /* Copy kernel mappings (first 2GB) */
    if (kernel_page_directory) {
        for (int i = 0; i < 512; i++) {
            page_dir[i] = kernel_page_directory[i];
        }
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
    
    /* Copy user space page directory entries (512-1024) */
    for (int i = 512; i < 1024; i++) {
        if (!(src[i] & PAGE_PRESENT)) continue;
        
        /* Get source page table */
        uint32_t* src_pt = (uint32_t*)PAGE_GET_PHYSICAL_ADDRESS(&src[i]);
        
        /* Allocate new page table */
        uint32_t pt_phys = alloc_frame();
        if (!pt_phys) continue;
        
        uint32_t* new_pt = (uint32_t*)pt_phys;
        
        /* Copy page table entries */
        for (int j = 0; j < 1024; j++) {
            if (!(src_pt[j] & PAGE_PRESENT)) {
                new_pt[j] = 0;
                continue;
            }
            
            /* Allocate new physical frame */
            uint32_t new_frame = alloc_frame();
            if (!new_frame) {
                new_pt[j] = 0;
                continue;
            }
            
            /* Copy page contents */
            uint32_t* src_page = (uint32_t*)PAGE_GET_PHYSICAL_ADDRESS(&src_pt[j]);
            uint32_t* dst_page = (uint32_t*)new_frame;
            
            for (int k = 0; k < 1024; k++) {
                dst_page[k] = src_page[k];
            }
            
            /* Set up new page table entry */
            new_pt[j] = new_frame | (src_pt[j] & 0xFFF);
        }
        
        /* Set up new page directory entry */
        new_pd[i] = pt_phys | (src[i] & 0xFFF);
    }
    
    return new_pd;
}