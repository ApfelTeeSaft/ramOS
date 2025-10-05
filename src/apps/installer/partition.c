/* partition.c - Disk partitioning implementation */

#include "partition.h"
#include "../../api/libsys.h"

#define SECTOR_SIZE 512
#define MBR_PARTITION_TABLE_OFFSET 446
#define MBR_SIGNATURE_OFFSET 510

/* MBR partition entry */
typedef struct {
    uint8_t status;           /* 0x80 = bootable */
    uint8_t first_chs[3];     /* CHS of first sector */
    uint8_t type;             /* Partition type */
    uint8_t last_chs[3];      /* CHS of last sector */
    uint32_t first_lba;       /* LBA of first sector */
    uint32_t num_sectors;     /* Number of sectors */
} __attribute__((packed)) mbr_partition_t;

/* MBR structure */
typedef struct {
    uint8_t bootcode[446];
    mbr_partition_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_t;

/* Create MBR partition table */
int create_partition_table(const char* device) {
    int fd = sys_open(device, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    mbr_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    
    /* Set MBR signature */
    mbr.signature = 0xAA55;
    
    /* Write MBR */
    if (sys_write(fd, &mbr, sizeof(mbr)) != sizeof(mbr)) {
        sys_close(fd);
        return -1;
    }
    
    sys_close(fd);
    return 0;
}

/* Create a partition */
int create_partition(const char* device, int partition_num,
                     uint32_t start_sector, uint32_t num_sectors) {
    if (partition_num < 1 || partition_num > 4) {
        return -1;
    }
    
    int fd = sys_open(device, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    /* Read existing MBR */
    mbr_t mbr;
    if (sys_read(fd, &mbr, sizeof(mbr)) != sizeof(mbr)) {
        sys_close(fd);
        return -1;
    }
    
    /* Verify MBR signature */
    if (mbr.signature != 0xAA55) {
        sys_close(fd);
        return -1;
    }
    
    /* Calculate partition parameters */
    int idx = partition_num - 1;
    
    /* Auto-calculate start sector */
    if (start_sector == 0) {
        start_sector = 2048; /* Default: 1MB offset */
        
        /* Find end of previous partition */
        for (int i = 0; i < idx; i++) {
            if (mbr.partitions[i].num_sectors > 0) {
                uint32_t end = mbr.partitions[i].first_lba + mbr.partitions[i].num_sectors;
                if (end > start_sector) {
                    start_sector = end;
                }
            }
        }
    }
    
    /* Auto-calculate number of sectors */
    if (num_sectors == 0) {
        uint32_t disk_size = get_disk_size(device);
        num_sectors = disk_size - start_sector;
    }
    
    /* Set up partition entry */
    mbr_partition_t* part = &mbr.partitions[idx];
    memset(part, 0, sizeof(mbr_partition_t));
    
    part->status = (partition_num == 1) ? 0x80 : 0x00; /* First partition is bootable */
    part->type = 0x83; /* Linux filesystem (EXT4) */
    part->first_lba = start_sector;
    part->num_sectors = num_sectors;
    
    /* Write updated MBR */
    sys_seek(fd, 0, SEEK_SET);
    if (sys_write(fd, &mbr, sizeof(mbr)) != sizeof(mbr)) {
        sys_close(fd);
        return -1;
    }
    
    sys_close(fd);
    return 0;
}

/* Get disk size in sectors */
uint32_t get_disk_size(const char* device) {
    /* Use IOCTL to get disk size */
    int fd = sys_open(device, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    uint32_t size = 0;
    sys_ioctl(fd, 0x1001, &size); /* IOCTL_BLOCK_SIZE */
    sys_close(fd);
    
    /* Default to 8GB if ioctl fails */
    if (size == 0) {
        size = 8 * 1024 * 1024 * 2; /* 8GB in sectors */
    }
    
    return size;
}