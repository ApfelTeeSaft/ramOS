/* initrd.h - Initial RAM disk filesystem */

#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include <stddef.h>

/* File entry structure */
typedef struct {
    char name[256];
    uint32_t size;
    uint8_t* data;
} initrd_file_t;

/* Initialize initrd from memory address */
int initrd_init(uint32_t addr, uint32_t size);

/* List all files */
int initrd_list(initrd_file_t** files);

/* Find file by name */
initrd_file_t* initrd_find(const char* name);

#endif /* INITRD_H */