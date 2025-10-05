/* partition.h - Disk partitioning functions */

#ifndef PARTITION_H
#define PARTITION_H

#include <stdint.h>

/* Create MBR partition table */
int create_partition_table(const char* device);

/* Create a partition 
 * partition_num: 1-4 for primary partitions
 * start_sector: Starting sector (0 = auto)
 * num_sectors: Number of sectors (0 = use remaining)
 */
int create_partition(const char* device, int partition_num, 
                    uint32_t start_sector, uint32_t num_sectors);

/* Get disk size in sectors */
uint32_t get_disk_size(const char* device);

#endif /* PARTITION_H */