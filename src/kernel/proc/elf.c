/* elf.c - ELF binary loader implementation */

#include "elf.h"
#include "../fs/vfs.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../core/console.h"

/* Memory copy helper */
static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

/* Memory set helper */
static void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

/* Validate ELF header */
int elf_validate(elf_header_t* header) {
    if (!header) return 0;
    
    /* Check magic number */
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        return 0;
    }
    
    /* Check for 32-bit */
    if (header->e_ident[4] != 1) {
        return 0;
    }
    
    /* Check for little endian */
    if (header->e_ident[5] != 1) {
        return 0;
    }
    
    /* Check for executable */
    if (header->e_type != 2) {
        return 0;
    }
    
    /* Check for x86 */
    if (header->e_machine != 3) {
        return 0;
    }
    
    return 1;
}

/* Load ELF binary from file */
uint32_t elf_load(const char* path) {
    /* Open file */
    int fd = vfs_open(path, 0);
    if (fd < 0) {
        kprintf("[ELF] Failed to open: %s\n", path);
        return 0;
    }
    
    /* Read ELF header */
    elf_header_t header;
    if (vfs_read(fd, &header, sizeof(header)) != sizeof(header)) {
        kprintf("[ELF] Failed to read header\n");
        vfs_close(fd);
        return 0;
    }
    
    /* Validate ELF header */
    if (!elf_validate(&header)) {
        kprintf("[ELF] Invalid ELF file\n");
        vfs_close(fd);
        return 0;
    }
    
    /* Read program headers */
    uint32_t phoff = header.e_phoff;
    uint16_t phnum = header.e_phnum;
    uint16_t phentsize = header.e_phentsize;
    
    for (int i = 0; i < phnum; i++) {
        elf_program_header_t phdr;
        
        /* Seek to program header */
        vfs_seek(fd, phoff + i * phentsize, 0);
        
        /* Read program header */
        if (vfs_read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            kprintf("[ELF] Failed to read program header %d\n", i);
            continue;
        }
        
        /* Only load PT_LOAD segments */
        if (phdr.p_type != PT_LOAD) {
            continue;
        }
        
        /* Allocate memory for segment */
        void* segment = (void*)phdr.p_vaddr;
        
        /* TODO: Map pages for segment in virtual memory */
        /* For now, just use physical addresses */
        
        /* Seek to segment data */
        vfs_seek(fd, phdr.p_offset, 0);
        
        /* Read segment data */
        if (phdr.p_filesz > 0) {
            if (vfs_read(fd, segment, phdr.p_filesz) != (int)phdr.p_filesz) {
                kprintf("[ELF] Failed to read segment %d\n", i);
                continue;
            }
        }
        
        /* Zero remaining memory if memsz > filesz (BSS) */
        if (phdr.p_memsz > phdr.p_filesz) {
            memset((uint8_t*)segment + phdr.p_filesz, 0, 
                   phdr.p_memsz - phdr.p_filesz);
        }
    }
    
    vfs_close(fd);
    
    /* Return entry point */
    return header.e_entry;
}