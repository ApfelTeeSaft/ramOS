/* format.c - Filesystem formatting implementation */

#include "format.h"
#include "../../api/libsys.h"

#define BLOCK_SIZE 4096
#define EXT4_SUPER_MAGIC 0xEF53

/* Simplified EXT4 superblock */
typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
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
    /* Additional fields for EXT4 */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
} __attribute__((packed)) ext4_superblock_t;

/* Format partition as EXT4 */
int format_ext4(const char* device, const char* label) {
    int fd = sys_open(device, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    /* Get device size */
    uint32_t device_size = 0;
    sys_ioctl(fd, 0x1001, &device_size);
    
    if (device_size == 0) {
        /* Default to reasonable size if ioctl fails */
        device_size = 256 * 1024 * 2; /* 256MB in sectors */
    }
    
    /* Calculate filesystem parameters */
    uint32_t block_count = (device_size * 512) / BLOCK_SIZE;
    uint32_t inode_count = block_count / 4; /* 1 inode per 4 blocks */
    
    /* Create superblock */
    ext4_superblock_t sb;
    memset(&sb, 0, sizeof(sb));
    
    sb.s_inodes_count = inode_count;
    sb.s_blocks_count = block_count;
    sb.s_r_blocks_count = block_count / 20; /* 5% reserved */
    sb.s_free_blocks_count = block_count - 100; /* Reserve some for metadata */
    sb.s_free_inodes_count = inode_count - 10; /* Reserve some inodes */
    sb.s_first_data_block = 1;
    sb.s_log_block_size = 2; /* 4096 bytes */
    sb.s_log_frag_size = 2;
    sb.s_blocks_per_group = 32768;
    sb.s_frags_per_group = 32768;
    sb.s_inodes_per_group = 8192;
    sb.s_mtime = 0;
    sb.s_wtime = 0;
    sb.s_mnt_count = 0;
    sb.s_max_mnt_count = 20;
    sb.s_magic = EXT4_SUPER_MAGIC;
    sb.s_state = 1; /* Clean */
    sb.s_errors = 1; /* Continue on errors */
    sb.s_minor_rev_level = 0;
    sb.s_lastcheck = 0;
    sb.s_checkinterval = 0;
    sb.s_creator_os = 0; /* Linux */
    sb.s_rev_level = 1; /* Dynamic revision */
    sb.s_def_resuid = 0;
    sb.s_def_resgid = 0;
    sb.s_first_ino = 11;
    sb.s_inode_size = 256;
    sb.s_block_group_nr = 0;
    sb.s_feature_compat = 0x38; /* dir_prealloc, has_journal, ext_attr */
    sb.s_feature_incompat = 0x2C2; /* filetype, extent, 64bit, flex_bg */
    sb.s_feature_ro_compat = 0x73; /* sparse_super, large_file, huge_file, dir_nlink */
    
    /* Set volume label */
    if (label) {
        strncpy(sb.s_volume_name, label, 16);
        sb.s_volume_name[15] = '\0';
    }
    
    /* Generate UUID (simplified) */
    for (int i = 0; i < 16; i++) {
        sb.s_uuid[i] = i * 17; /* Simple pattern */
    }
    
    /* Write superblock at offset 1024 bytes */
    uint8_t buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    
    sys_seek(fd, 1024, SEEK_SET);
    memcpy(buffer, &sb, sizeof(sb));
    
    if (sys_write(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        sys_close(fd);
        return -1;
    }
    
    /* Write backup superblock */
    sys_seek(fd, BLOCK_SIZE * 32768, SEEK_SET);
    sys_write(fd, buffer, BLOCK_SIZE);
    
    /* Initialize root directory (inode 2) */
    /* This is simplified - a real implementation would set up
     * the full inode table and directory structure */
    
    sys_close(fd);
    return 0;
}