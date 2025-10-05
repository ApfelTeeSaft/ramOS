/* vfs.h - Virtual File System */

#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

/* VFS node types */
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08

/* Forward declarations */
struct vfs_node;

/* VFS operations */
typedef int (*vfs_read_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef int (*vfs_write_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*vfs_open_t)(struct vfs_node*);
typedef void (*vfs_close_t)(struct vfs_node*);
typedef struct vfs_node* (*vfs_readdir_t)(struct vfs_node*, uint32_t);
typedef struct vfs_node* (*vfs_finddir_t)(struct vfs_node*, const char*);

/* VFS node structure */
typedef struct vfs_node {
    char name[128];              /* Filename */
    uint32_t mask;               /* Permissions mask */
    uint32_t uid;                /* User ID */
    uint32_t gid;                /* Group ID */
    uint32_t flags;              /* Node type flags */
    uint32_t inode;              /* Inode number */
    uint32_t length;             /* File size */
    uint32_t impl;               /* Implementation-defined */
    
    vfs_read_t read;
    vfs_write_t write;
    vfs_open_t open;
    vfs_close_t close;
    vfs_readdir_t readdir;
    vfs_finddir_t finddir;
    
    struct vfs_node* ptr;        /* Used by mountpoints and symlinks */
} vfs_node_t;

/* Directory entry */
typedef struct {
    char name[128];
    uint32_t inode;
} dirent_t;

/* File descriptor */
typedef struct {
    vfs_node_t* node;
    uint32_t position;
    uint32_t flags;
} file_descriptor_t;

/* Initialize VFS */
void vfs_init(void);

/* File operations */
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);
int vfs_seek(int fd, int offset, int whence);

/* Directory operations */
int vfs_readdir(int fd, void* entry);
vfs_node_t* vfs_finddir(const char* path);
int vfs_mkdir(const char* path, uint32_t mode);
int vfs_rmdir(const char* path);
int vfs_unlink(const char* path);

/* Mounting */
int vfs_mount(const char* source, const char* target, const char* fstype);
int vfs_umount(const char* target);

/* Stat */
int vfs_stat(const char* path, void* statbuf);

#endif /* VFS_H */