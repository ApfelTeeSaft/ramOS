/* vfs.c - Virtual File System implementation */

#include "vfs.h"
#include "initrd.h"
#include "../mm/heap.h"
#include "../core/console.h"

#define MAX_FILE_DESCRIPTORS 256

static file_descriptor_t fd_table[MAX_FILE_DESCRIPTORS];
static vfs_node_t* root_node = NULL;

/* String utilities */
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

static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

/* Initialize VFS */
void vfs_init(void) {
    kprintf("[VFS] Initializing Virtual File System...\n");
    
    /* Clear file descriptor table */
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        fd_table[i].node = NULL;
        fd_table[i].position = 0;
        fd_table[i].flags = 0;
    }
    
    /* Initialize initrd as root filesystem */
    root_node = initrd_get_root();
}

/* Allocate file descriptor */
static int alloc_fd(void) {
    for (int i = 3; i < MAX_FILE_DESCRIPTORS; i++) { /* Skip stdin/stdout/stderr */
        if (fd_table[i].node == NULL) {
            return i;
        }
    }
    return -1;
}

/* Open file */
int vfs_open(const char* path, int flags) {
    if (!path) return -1;
    
    /* Find node */
    vfs_node_t* node = vfs_finddir(path);
    if (!node) return -1;
    
    /* Allocate file descriptor */
    int fd = alloc_fd();
    if (fd < 0) return -1;
    
    /* Set up file descriptor */
    fd_table[fd].node = node;
    fd_table[fd].position = 0;
    fd_table[fd].flags = flags;
    
    /* Call open callback if it exists */
    if (node->open) {
        node->open(node);
    }
    
    return fd;
}

/* Close file */
int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    /* Call close callback if it exists */
    if (node->close) {
        node->close(node);
    }
    
    /* Clear file descriptor */
    fd_table[fd].node = NULL;
    fd_table[fd].position = 0;
    fd_table[fd].flags = 0;
    
    return 0;
}

/* Read from file */
int vfs_read(int fd, void* buffer, size_t size) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    if (!node->read) {
        return -1;
    }
    
    int bytes_read = node->read(node, fd_table[fd].position, size, (uint8_t*)buffer);
    if (bytes_read > 0) {
        fd_table[fd].position += bytes_read;
    }
    
    return bytes_read;
}

/* Write to file */
int vfs_write(int fd, const void* buffer, size_t size) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    if (!node->write) {
        return -1;
    }
    
    int bytes_written = node->write(node, fd_table[fd].position, size, (uint8_t*)buffer);
    if (bytes_written > 0) {
        fd_table[fd].position += bytes_written;
    }
    
    return bytes_written;
}

/* Seek in file */
int vfs_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    uint32_t new_pos;
    
    switch (whence) {
        case 0: /* SEEK_SET */
            new_pos = offset;
            break;
        case 1: /* SEEK_CUR */
            new_pos = fd_table[fd].position + offset;
            break;
        case 2: /* SEEK_END */
            new_pos = node->length + offset;
            break;
        default:
            return -1;
    }
    
    fd_table[fd].position = new_pos;
    return new_pos;
}

/* Read directory entry */
int vfs_readdir(int fd, void* entry) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    if (!(node->flags & VFS_DIRECTORY) || !node->readdir) {
        return -1;
    }
    
    uint32_t index = fd_table[fd].position;
    vfs_node_t* child = node->readdir(node, index);
    
    if (!child) {
        return 0; /* End of directory */
    }
    
    dirent_t* de = (dirent_t*)entry;
    strncpy(de->name, child->name, 128);
    de->inode = child->inode;
    
    fd_table[fd].position++;
    
    return 1;
}

/* Find directory/file by path */
vfs_node_t* vfs_finddir(const char* path) {
    if (!path || !root_node) return NULL;
    
    /* Handle root */
    if (strcmp(path, "/") == 0) {
        return root_node;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') path++;
    
    vfs_node_t* current = root_node;
    char component[128];
    int comp_idx = 0;
    
    while (*path) {
        /* Extract path component */
        if (*path == '/') {
            component[comp_idx] = '\0';
            comp_idx = 0;
            
            /* Find component in current directory */
            if (current->finddir) {
                current = current->finddir(current, component);
                if (!current) return NULL;
            } else {
                return NULL;
            }
            
            path++;
        } else {
            if (comp_idx < 127) {
                component[comp_idx++] = *path;
            }
            path++;
        }
    }
    
    /* Handle last component */
    if (comp_idx > 0) {
        component[comp_idx] = '\0';
        if (current->finddir) {
            current = current->finddir(current, component);
        } else {
            return NULL;
        }
    }
    
    return current;
}

/* Stat file */
int vfs_stat(const char* path, void* statbuf) {
    vfs_node_t* node = vfs_finddir(path);
    if (!node) return -1;
    
    /* Fill stat buffer (simplified) */
    typedef struct {
        uint32_t st_mode;
        uint32_t st_size;
        uint32_t st_blocks;
        uint32_t st_atime;
        uint32_t st_mtime;
        uint32_t st_ctime;
    } stat_t;
    
    stat_t* st = (stat_t*)statbuf;
    st->st_mode = node->flags;
    st->st_size = node->length;
    st->st_blocks = (node->length + 511) / 512;
    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0;
    
    return 0;
}

/* Stub implementations for filesystem operations */
int vfs_mkdir(const char* path, uint32_t mode) {
    (void)path;
    (void)mode;
    return -1; /* Not implemented */
}

int vfs_rmdir(const char* path) {
    (void)path;
    return -1; /* Not implemented */
}

int vfs_unlink(const char* path) {
    (void)path;
    return -1; /* Not implemented */
}

int vfs_mount(const char* source, const char* target, const char* fstype) {
    (void)source;
    (void)target;
    (void)fstype;
    return -1; /* Not implemented */
}

int vfs_umount(const char* target) {
    (void)target;
    return -1; /* Not implemented */
}