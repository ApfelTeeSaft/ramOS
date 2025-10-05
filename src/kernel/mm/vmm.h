/* vmm.h - Virtual Memory Manager */

#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* Page flags */
#define PAGE_PRESENT   0x1
#define PAGE_WRITE     0x2
#define PAGE_USER      0x4

/* Page size */
#define PAGE_SIZE 4096

/* Initialize virtual memory */
void vmm_init(void);

/* Map virtual address to physical address */
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* Unmap virtual address */
void vmm_unmap_page(uint32_t virtual_addr);

/* Get physical address from virtual address */
uint32_t vmm_get_physical(uint32_t virtual_addr);

/* Create new page directory */
uint32_t* vmm_create_page_directory(void);

/* Switch page directory */
void vmm_switch_page_directory(uint32_t* page_directory);

/* Get current page directory */
uint32_t* vmm_get_page_directory(void);

#endif /* VMM_H */