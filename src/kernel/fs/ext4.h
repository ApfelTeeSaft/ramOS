/* ext4.h - EXT4 filesystem driver */

#ifndef EXT4_H
#define EXT4_H

#include <stdint.h>
#include "vfs.h"

/* EXT4 superblock magic */
#define EXT4_SUPER_MAGIC 0xEF53

/* EXT4 inode constants */
#define EXT4_ROOT_INO     2
#define EXT4_GOOD_OLD_INODE_SIZE 128

/* EXT4 file types */
#define EXT4_FT_UNKNOWN  0
#define EXT4_FT_REG_FILE 1
#define EXT4_FT_DIR      2
#define EXT4_FT_CHRDEV   3
#define EXT4_FT_BLKDEV   4
#define EXT4_FT_FIFO     5
#define EXT4_FT_SOCK     6
#define EXT4_FT_SYMLINK  7

/* EXT4 superblock structure (simplified) */
typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count_lo;
    uint32_t s_r_blocks_count_lo;
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_cluster_size;
    uint32_t s_blocks_per_group;
    uint32_t s_clusters_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    /* Extended fields */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
} __attribute__((packed)) ext4_superblock_t;

/* EXT4 inode structure (simplified) */
typedef struct {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size_lo;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks_lo;
    uint32_t i_flags;
    uint32_t i_block[15];
} __attribute__((packed)) ext4_inode_t;

/* Initialize EXT4 filesystem support */
int ext4_init(void);

/* Mount EXT4 filesystem from device */
vfs_node_t* ext4_mount(const char* device);

/* Unmount EXT4 filesystem */
int ext4_umount(vfs_node_t* node);

/* Read EXT4 superblock */
int ext4_read_superblock(const char* device, ext4_superblock_t* sb);

/* Read EXT4 inode */
int ext4_read_inode(vfs_node_t* fs_root, uint32_t inode_num, ext4_inode_t* inode);

#endif /* EXT4_H */