/* elf.h - ELF binary loader */

#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/* ELF file header */
typedef struct {
    uint8_t  e_ident[16];    /* Magic number and other info */
    uint16_t e_type;         /* Object file type */
    uint16_t e_machine;      /* Architecture */
    uint32_t e_version;      /* Object file version */
    uint32_t e_entry;        /* Entry point virtual address */
    uint32_t e_phoff;        /* Program header table file offset */
    uint32_t e_shoff;        /* Section header table file offset */
    uint32_t e_flags;        /* Processor-specific flags */
    uint16_t e_ehsize;       /* ELF header size in bytes */
    uint16_t e_phentsize;    /* Program header table entry size */
    uint16_t e_phnum;        /* Program header table entry count */
    uint16_t e_shentsize;    /* Section header table entry size */
    uint16_t e_shnum;        /* Section header table entry count */
    uint16_t e_shstrndx;     /* Section header string table index */
} __attribute__((packed)) elf_header_t;

/* ELF program header */
typedef struct {
    uint32_t p_type;         /* Segment type */
    uint32_t p_offset;       /* Segment file offset */
    uint32_t p_vaddr;        /* Segment virtual address */
    uint32_t p_paddr;        /* Segment physical address */
    uint32_t p_filesz;       /* Segment size in file */
    uint32_t p_memsz;        /* Segment size in memory */
    uint32_t p_flags;        /* Segment flags */
    uint32_t p_align;        /* Segment alignment */
} __attribute__((packed)) elf_program_header_t;

/* ELF magic numbers */
#define ELF_MAGIC 0x464C457F  /* "\x7FELF" */

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

/* Load ELF binary from file and return entry point */
uint32_t elf_load(const char* path);

/* Validate ELF header */
int elf_validate(elf_header_t* header);

#endif /* ELF_H */