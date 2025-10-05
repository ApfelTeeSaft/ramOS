/* initrd.h - Initial RAM disk filesystem */

#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include <stddef.h>

/* Forward declare vfs_node_t to avoid circular dependency */
struct vfs_node;

/* File entry structure */
typedef struct {
    char name[256];
    uint32_t size;
    uint8_t* data;
    struct vfs_node* vfs_node;
} initrd_file_t;

/* Initialize initrd from memory address */
int initrd_init(uint32_t addr, uint32_t size);

/* Get root VFS node */
struct vfs_node* initrd_get_root(void);

/* Legacy compatibility functions */
int initrd_list(initrd_file_t** files);
initrd_file_t* initrd_find(const char* name);

#endif /* INITRD_H */