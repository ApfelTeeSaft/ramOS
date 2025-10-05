/* vfs.c - Complete Virtual File System Implementation */

#include "vfs.h"
#include "initrd.h"
#include "path.h"
#include "../mm/heap.h"
#include "../core/console.h"

#define MAX_FILE_DESCRIPTORS 256
#define MAX_MOUNTS 16
#define MAX_PATH_LENGTH 512

static file_descriptor_t fd_table[MAX_FILE_DESCRIPTORS];
static vfs_node_t* root_node = NULL;

/* Mount point structure */
typedef struct mount_point {
    char path[256];
    vfs_node_t* node;
    char fstype[32];
    char source[256];
    struct mount_point* next;
} mount_point_t;

static mount_point_t* mount_list = NULL;

/* String utilities */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
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

static char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
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
    
    /* Initialize mount list */
    mount_list = NULL;
    
    /* Initialize initrd as root filesystem */
    root_node = initrd_get_root();
    
    if (root_node) {
        kprintf("[VFS] Root filesystem mounted (initrd)\n");
        
        /* Create root mount point */
        mount_point_t* root_mount = (mount_point_t*)kmalloc(sizeof(mount_point_t));
        if (root_mount) {
            strcpy(root_mount->path, "/");
            strcpy(root_mount->fstype, "initrd");
            strcpy(root_mount->source, "initrd");
            root_mount->node = root_node;
            root_mount->next = NULL;
            mount_list = root_mount;
        }
    }
}

/* Find mount point for path */
static mount_point_t* find_mount_point(const char* path) {
    if (!path) return NULL;
    
    mount_point_t* best_match = NULL;
    size_t best_len = 0;
    
    /* Find longest matching mount point */
    for (mount_point_t* mp = mount_list; mp != NULL; mp = mp->next) {
        size_t len = strlen(mp->path);
        
        if (strncmp(path, mp->path, len) == 0 && len >= best_len) {
            /* Make sure it's a directory boundary */
            if (path[len] == '/' || path[len] == '\0' || strcmp(mp->path, "/") == 0) {
                best_match = mp;
                best_len = len;
            }
        }
    }
    
    return best_match;
}

/* Get relative path after mount point */
static const char* get_relative_path(const char* path, mount_point_t* mp) {
    if (!path || !mp) return path;
    
    size_t mp_len = strlen(mp->path);
    
    /* Skip mount point path */
    const char* rel = path + mp_len;
    
    /* Skip leading slashes */
    while (*rel == '/') rel++;
    
    return *rel ? rel : ".";
}

/* Resolve path to VFS node */
vfs_node_t* vfs_finddir(const char* path) {
    if (!path) return NULL;
    
    /* Handle root */
    if (strcmp(path, "/") == 0) {
        return root_node;
    }
    
    /* Find mount point */
    mount_point_t* mp = find_mount_point(path);
    if (!mp) {
        kprintf("[VFS] No mount point for: %s\n", path);
        return NULL;
    }
    
    /* Get relative path */
    const char* rel_path = get_relative_path(path, mp);
    
    /* If we're at mount root, return mount node */
    if (strcmp(rel_path, ".") == 0 || *rel_path == '\0') {
        return mp->node;
    }
    
    /* Traverse path components */
    vfs_node_t* current = mp->node;
    char component[128];
    int comp_idx = 0;
    
    while (*rel_path) {
        if (*rel_path == '/') {
            if (comp_idx > 0) {
                component[comp_idx] = '\0';
                comp_idx = 0;
                
                /* Handle special directories */
                if (strcmp(component, ".") == 0) {
                    /* Stay in current directory */
                } else if (strcmp(component, "..") == 0) {
                    /* Go to parent */
                    /* TODO: Implement parent directory tracking */
                    kprintf("[VFS] Warning: .. navigation not fully implemented\n");
                } else {
                    /* Find component */
                    if (!current || !(current->flags & VFS_DIRECTORY)) {
                        return NULL;
                    }
                    
                    if (current->finddir) {
                        current = current->finddir(current, component);
                        if (!current) {
                            return NULL;
                        }
                    } else {
                        return NULL;
                    }
                }
            }
            rel_path++;
        } else {
            if (comp_idx < 127) {
                component[comp_idx++] = *rel_path;
            }
            rel_path++;
        }
    }
    
    /* Handle last component */
    if (comp_idx > 0) {
        component[comp_idx] = '\0';
        
        if (strcmp(component, ".") == 0) {
            /* Current directory */
        } else if (strcmp(component, "..") == 0) {
            /* Parent directory */
        } else {
            if (!current || !(current->flags & VFS_DIRECTORY)) {
                return NULL;
            }
            
            if (current->finddir) {
                current = current->finddir(current, component);
            } else {
                return NULL;
            }
        }
    }
    
    return current;
}

/* Allocate file descriptor */
static int alloc_fd(void) {
    /* Reserve 0, 1, 2 for stdin, stdout, stderr */
    for (int i = 3; i < MAX_FILE_DESCRIPTORS; i++) {
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
    if (!node) {
        /* File doesn't exist */
        if (flags & O_CREAT) {
            /* TODO: Create new file */
            kprintf("[VFS] File creation not yet implemented\n");
            return -1;
        }
        return -1;
    }
    
    /* Check if it's a directory (can't open for writing) */
    if ((node->flags & VFS_DIRECTORY) && (flags & (O_WRONLY | O_RDWR))) {
        return -1;
    }
    
    /* Allocate file descriptor */
    int fd = alloc_fd();
    if (fd < 0) {
        return -1;
    }
    
    /* Set up file descriptor */
    fd_table[fd].node = node;
    fd_table[fd].position = 0;
    fd_table[fd].flags = flags;
    
    /* Truncate if requested */
    if (flags & O_TRUNC) {
        /* TODO: Truncate file */
    }
    
    /* Call open callback if exists */
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
    
    /* Call close callback if exists */
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
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node || !buffer) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    /* Check read permission */
    if (fd_table[fd].flags & O_WRONLY) {
        return -1;  /* File opened write-only */
    }
    
    /* Can't read directories */
    if (node->flags & VFS_DIRECTORY) {
        return -1;
    }
    
    if (!node->read) {
        return -1;
    }
    
    /* Perform read */
    int bytes_read = node->read(node, fd_table[fd].position, size, (uint8_t*)buffer);
    
    if (bytes_read > 0) {
        fd_table[fd].position += bytes_read;
    }
    
    return bytes_read;
}

/* Write to file */
int vfs_write(int fd, const void* buffer, size_t size) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node || !buffer) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    /* Check write permission */
    if (fd_table[fd].flags & O_RDONLY) {
        return -1;  /* File opened read-only */
    }
    
    /* Can't write to directories */
    if (node->flags & VFS_DIRECTORY) {
        return -1;
    }
    
    if (!node->write) {
        return -1;
    }
    
    /* Handle append mode */
    if (fd_table[fd].flags & O_APPEND) {
        fd_table[fd].position = node->length;
    }
    
    /* Perform write */
    int bytes_written = node->write(node, fd_table[fd].position, size, (uint8_t*)buffer);
    
    if (bytes_written > 0) {
        fd_table[fd].position += bytes_written;
        
        /* Update file length if we extended it */
        if (fd_table[fd].position > node->length) {
            node->length = fd_table[fd].position;
        }
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
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = fd_table[fd].position + offset;
            break;
        case SEEK_END:
            new_pos = node->length + offset;
            break;
        default:
            return -1;
    }
    
    /* Don't allow seeking before start */
    if ((int)new_pos < 0) {
        return -1;
    }
    
    fd_table[fd].position = new_pos;
    return new_pos;
}

/* Read directory entry */
int vfs_readdir(int fd, void* entry) {
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS || !fd_table[fd].node || !entry) {
        return -1;
    }
    
    vfs_node_t* node = fd_table[fd].node;
    
    /* Must be a directory */
    if (!(node->flags & VFS_DIRECTORY)) {
        return -1;
    }
    
    if (!node->readdir) {
        return -1;
    }
    
    uint32_t index = fd_table[fd].position;
    vfs_node_t* child = node->readdir(node, index);
    
    if (!child) {
        return 0;  /* End of directory */
    }
    
    /* Fill dirent structure */
    dirent_t* de = (dirent_t*)entry;
    strncpy(de->name, child->name, 128);
    de->name[127] = '\0';
    de->inode = child->inode;
    
    fd_table[fd].position++;
    
    return 1;
}

/* Get file/directory statistics */
int vfs_stat(const char* path, void* statbuf) {
    if (!path || !statbuf) return -1;
    
    vfs_node_t* node = vfs_finddir(path);
    if (!node) return -1;
    
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
    st->st_atime = 0;  /* TODO: Implement timestamps */
    st->st_mtime = 0;
    st->st_ctime = 0;
    
    return 0;
}

/* Mount filesystem */
int vfs_mount(const char* source, const char* target, const char* fstype) {
    if (!source || !target || !fstype) return -1;
    
    kprintf("[VFS] Mounting %s on %s (type: %s)\n", source, target, fstype);
    
    /* Check if target exists and is a directory */
    vfs_node_t* target_node = vfs_finddir(target);
    if (target_node && !(target_node->flags & VFS_DIRECTORY)) {
        kprintf("[VFS] Mount target is not a directory\n");
        return -1;
    }
    
    /* Check if already mounted */
    for (mount_point_t* mp = mount_list; mp != NULL; mp = mp->next) {
        if (strcmp(mp->path, target) == 0) {
            kprintf("[VFS] Target already mounted\n");
            return -1;
        }
    }
    
    /* Create mount point structure */
    mount_point_t* mp = (mount_point_t*)kmalloc(sizeof(mount_point_t));
    if (!mp) return -1;
    
    strncpy(mp->path, target, 256);
    strncpy(mp->fstype, fstype, 32);
    strncpy(mp->source, source, 256);
    mp->node = NULL;
    
    /* Load filesystem driver based on type */
    if (strcmp(fstype, "ext4") == 0) {
        extern vfs_node_t* ext4_mount(const char*);
        mp->node = ext4_mount(source);
    } else if (strcmp(fstype, "initrd") == 0) {
        mp->node = root_node;
    } else {
        kprintf("[VFS] Unknown filesystem type: %s\n", fstype);
        kfree(mp);
        return -1;
    }
    
    if (!mp->node) {
        kprintf("[VFS] Failed to mount filesystem\n");
        kfree(mp);
        return -1;
    }
    
    /* Add to mount list */
    mp->next = mount_list;
    mount_list = mp;
    
    kprintf("[VFS] Mounted successfully\n");
    
    return 0;
}

/* Unmount filesystem */
int vfs_umount(const char* target) {
    if (!target) return -1;
    
    kprintf("[VFS] Unmounting: %s\n", target);
    
    /* Find mount point */
    mount_point_t** current = &mount_list;
    while (*current) {
        if (strcmp((*current)->path, target) == 0) {
            mount_point_t* mp = *current;
            
            /* Can't unmount root */
            if (strcmp(mp->path, "/") == 0) {
                kprintf("[VFS] Cannot unmount root filesystem\n");
                return -1;
            }
            
            /* Remove from list */
            *current = mp->next;
            
            /* Call filesystem-specific unmount */
            if (strcmp(mp->fstype, "ext4") == 0) {
                extern int ext4_umount(vfs_node_t*);
                ext4_umount(mp->node);
            }
            
            kfree(mp);
            kprintf("[VFS] Unmounted successfully\n");
            return 0;
        }
        current = &(*current)->next;
    }
    
    kprintf("[VFS] Mount point not found\n");
    return -1;
}

/* Create directory */
int vfs_mkdir(const char* path, uint32_t mode) {
    if (!path) return -1;
    
    kprintf("[VFS] Creating directory: %s (mode: %o)\n", path, mode);
    
    /* Find parent directory */
    char* parent_path = path_dirname(path);
    char* dir_name = path_basename(path);
    
    if (!parent_path || !dir_name) {
        kprintf("[VFS] Invalid path\n");
        return -1;
    }
    
    vfs_node_t* parent = vfs_finddir(parent_path);
    kfree(parent_path);
    
    if (!parent) {
        kprintf("[VFS] Parent directory not found\n");
        kfree(dir_name);
        return -1;
    }
    
    if (!(parent->flags & VFS_DIRECTORY)) {
        kprintf("[VFS] Parent is not a directory\n");
        kfree(dir_name);
        return -1;
    }
    
    /* TODO: Implement directory creation in filesystem */
    /* For now, this is filesystem-specific */
    
    kfree(dir_name);
    kprintf("[VFS] mkdir not fully implemented for this filesystem\n");
    return -1;
}

/* Remove directory */
int vfs_rmdir(const char* path) {
    if (!path) return -1;
    
    kprintf("[VFS] Removing directory: %s\n", path);
    
    vfs_node_t* node = vfs_finddir(path);
    if (!node) {
        return -1;
    }
    
    if (!(node->flags & VFS_DIRECTORY)) {
        kprintf("[VFS] Not a directory\n");
        return -1;
    }
    
    /* TODO: Check if directory is empty */
    /* TODO: Implement directory removal in filesystem */
    
    kprintf("[VFS] rmdir not fully implemented\n");
    return -1;
}

/* Remove file */
int vfs_unlink(const char* path) {
    if (!path) return -1;
    
    kprintf("[VFS] Unlinking: %s\n", path);
    
    vfs_node_t* node = vfs_finddir(path);
    if (!node) {
        return -1;
    }
    
    if (node->flags & VFS_DIRECTORY) {
        kprintf("[VFS] Is a directory (use rmdir)\n");
        return -1;
    }
    
    /* TODO: Implement file removal in filesystem */
    
    kprintf("[VFS] unlink not fully implemented\n");
    return -1;
}

/* Duplicate file descriptor */
int vfs_dup(int oldfd) {
    if (oldfd < 0 || oldfd >= MAX_FILE_DESCRIPTORS || !fd_table[oldfd].node) {
        return -1;
    }
    
    int newfd = alloc_fd();
    if (newfd < 0) {
        return -1;
    }
    
    /* Copy FD entry */
    fd_table[newfd].node = fd_table[oldfd].node;
    fd_table[newfd].position = fd_table[oldfd].position;
    fd_table[newfd].flags = fd_table[oldfd].flags;
    
    return newfd;
}

/* Duplicate file descriptor to specific FD */
int vfs_dup2(int oldfd, int newfd) {
    if (oldfd < 0 || oldfd >= MAX_FILE_DESCRIPTORS || !fd_table[oldfd].node) {
        return -1;
    }
    
    if (newfd < 0 || newfd >= MAX_FILE_DESCRIPTORS) {
        return -1;
    }
    
    if (oldfd == newfd) {
        return newfd;
    }
    
    /* Close newfd if it's open */
    if (fd_table[newfd].node) {
        vfs_close(newfd);
    }
    
    /* Copy FD entry */
    fd_table[newfd].node = fd_table[oldfd].node;
    fd_table[newfd].position = fd_table[oldfd].position;
    fd_table[newfd].flags = fd_table[oldfd].flags;
    
    return newfd;
}

/* List all mount points */
void vfs_list_mounts(void) {
    kprintf("[VFS] Mount Points:\n");
    kprintf("  %-20s %-20s %-10s\n", "TARGET", "SOURCE", "TYPE");
    kprintf("  %-20s %-20s %-10s\n", "------", "------", "----");
    
    for (mount_point_t* mp = mount_list; mp != NULL; mp = mp->next) {
        kprintf("  %-20s %-20s %-10s\n", mp->path, mp->source, mp->fstype);
    }
}

/* Get file descriptor info (for debugging) */
void vfs_list_fds(void) {
    kprintf("[VFS] Open File Descriptors:\n");
    kprintf("  FD   FLAGS  POS      NAME\n");
    kprintf("  ---  -----  -------  ----\n");
    
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (fd_table[i].node) {
            kprintf("  %-3d  0x%03x  %-7u  %s\n", 
                   i, fd_table[i].flags, fd_table[i].position,
                   fd_table[i].node->name);
        }
    }
}

/* Check if file exists */
int vfs_exists(const char* path) {
    return vfs_finddir(path) != NULL;
}

/* Check if path is a directory */
int vfs_isdir(const char* path) {
    vfs_node_t* node = vfs_finddir(path);
    return node && (node->flags & VFS_DIRECTORY);
}

/* Check if path is a regular file */
int vfs_isfile(const char* path) {
    vfs_node_t* node = vfs_finddir(path);
    return node && (node->flags & VFS_FILE);
}