/* initrd.c - Initial RAM disk with VFS integration */

#include "initrd.h"
#include "vfs.h"
#include "../mm/heap.h"

#define MAX_FILES 64

/* Initrd file structure */
typedef struct {
    char name[256];
    uint32_t size;
    uint8_t* data;
    vfs_node_t* vfs_node;
} initrd_file_t;

static initrd_file_t files[MAX_FILES];
static int file_count = 0;
static vfs_node_t* initrd_root = NULL;
static vfs_node_t* initrd_nodes[MAX_FILES];

/* String functions */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

/* Parse hex string */
static uint32_t parse_hex(const char* str, int len) {
    uint32_t val = 0;
    for (int i = 0; i < len; i++) {
        val <<= 4;
        char c = str[i];
        if (c >= '0' && c <= '9') {
            val += c - '0';
        } else if (c >= 'A' && c <= 'F') {
            val += c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            val += c - 'a' + 10;
        }
    }
    return val;
}

/* VFS read callback */
static int initrd_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    initrd_file_t* file = (initrd_file_t*)node->impl;
    if (!file) return 0;
    
    if (offset >= file->size) return 0;
    if (offset + size > file->size) {
        size = file->size - offset;
    }
    
    memcpy(buffer, file->data + offset, size);
    return size;
}

/* VFS readdir callback */
static vfs_node_t* initrd_readdir(vfs_node_t* node, uint32_t index) {
    (void)node;
    
    if (index >= (uint32_t)file_count) {
        return NULL;
    }
    
    return initrd_nodes[index];
}

/* VFS finddir callback */
static vfs_node_t* initrd_finddir(vfs_node_t* node, const char* name) {
    (void)node;
    
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return initrd_nodes[i];
        }
    }
    
    return NULL;
}

/* Initialize initrd from memory address */
int initrd_init(uint32_t addr, uint32_t size) {
    uint8_t* data = (uint8_t*)addr;
    uint8_t* end = data + size;
    
    file_count = 0;
    
    while (data < end && file_count < MAX_FILES) {
        /* Check for cpio newc magic */
        if (data + 110 > end) break;
        
        if (data[0] != '0' || data[1] != '7' || data[2] != '0' ||
            data[3] != '7' || data[4] != '0' || data[5] != '1') {
            break;
        }
        
        /* Parse header */
        uint32_t namesize = parse_hex((char*)(data + 94), 8);
        uint32_t filesize = parse_hex((char*)(data + 54), 8);
        
        /* Get filename */
        char* filename = (char*)(data + 110);
        
        /* Check for trailer */
        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }
        
        /* Skip . and empty names */
        if (filename[0] == '.' && (filename[1] == '\0' || filename[1] == '/')) {
            uint32_t header_size = ((110 + namesize + 3) & ~3);
            uint32_t data_size = ((filesize + 3) & ~3);
            data += header_size + data_size;
            continue;
        }
        
        /* Store file info */
        if (file_count < MAX_FILES) {
            initrd_file_t* f = &files[file_count];
            
            /* Copy filename (skip leading ./) */
            const char* name = filename;
            if (name[0] == '.' && name[1] == '/') {
                name += 2;
            }
            
            size_t name_len = strlen(name);
            if (name_len >= sizeof(f->name)) {
                name_len = sizeof(f->name) - 1;
            }
            memcpy(f->name, name, name_len);
            f->name[name_len] = '\0';
            
            f->size = filesize;
            
            /* Align to 4 bytes */
            uint32_t header_size = ((110 + namesize + 3) & ~3);
            f->data = data + header_size;
            
            /* Create VFS node */
            vfs_node_t* vnode = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
            if (vnode) {
                strncpy(vnode->name, f->name, 128);
                vnode->mask = 0;
                vnode->uid = 0;
                vnode->gid = 0;
                vnode->flags = VFS_FILE;
                vnode->inode = file_count;
                vnode->length = f->size;
                vnode->impl = (uint32_t)f;
                vnode->read = initrd_read;
                vnode->write = NULL;
                vnode->open = NULL;
                vnode->close = NULL;
                vnode->readdir = NULL;
                vnode->finddir = NULL;
                vnode->ptr = NULL;
                
                f->vfs_node = vnode;
                initrd_nodes[file_count] = vnode;
            }
            
            file_count++;
            
            /* Move to next entry */
            uint32_t data_size = ((filesize + 3) & ~3);
            data += header_size + data_size;
        } else {
            break;
        }
    }
    
    /* Create root directory node */
    initrd_root = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (initrd_root) {
        strncpy(initrd_root->name, "initrd", 128);
        initrd_root->mask = 0;
        initrd_root->uid = 0;
        initrd_root->gid = 0;
        initrd_root->flags = VFS_DIRECTORY;
        initrd_root->inode = 0;
        initrd_root->length = 0;
        initrd_root->impl = 0;
        initrd_root->read = NULL;
        initrd_root->write = NULL;
        initrd_root->open = NULL;
        initrd_root->close = NULL;
        initrd_root->readdir = initrd_readdir;
        initrd_root->finddir = initrd_finddir;
        initrd_root->ptr = NULL;
    }
    
    return file_count;
}

/* Get root VFS node */
vfs_node_t* initrd_get_root(void) {
    return initrd_root;
}

/* Legacy compatibility functions */
int initrd_list(initrd_file_t** out_files) {
    *out_files = files;
    return file_count;
}

initrd_file_t* initrd_find(const char* name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}