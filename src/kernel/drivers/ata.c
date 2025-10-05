/* ata.c - ATA/IDE Disk Driver Implementation */

#include "ata.h"
#include "driver.h"
#include "../core/console.h"

/* Drive information */
typedef struct {
    uint8_t exists;
    uint32_t size;          /* Size in sectors */
    uint16_t identify[256]; /* IDENTIFY data */
} ata_drive_info_t;

static ata_drive_info_t drives[4]; /* Primary master/slave, Secondary master/slave */

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void inw_buffer(uint16_t port, uint16_t* buffer, uint32_t count) {
    __asm__ volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

static inline void outw_buffer(uint16_t port, const uint16_t* buffer, uint32_t count) {
    __asm__ volatile("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

/* Wait for drive to be ready */
static int ata_wait_ready(uint16_t base) {
    uint32_t timeout = 100000;
    
    while (timeout--) {
        uint8_t status = inb(base + 7); /* Status port */
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRDY)) {
            return 0;
        }
    }
    
    return -1; /* Timeout */
}

/* Wait for DRQ (data request) */
static int ata_wait_drq(uint16_t base) {
    uint32_t timeout = 100000;
    
    while (timeout--) {
        uint8_t status = inb(base + 7);
        if (status & ATA_SR_DRQ) {
            return 0;
        }
        if (status & ATA_SR_ERR) {
            return -1;
        }
    }
    
    return -1; /* Timeout */
}

/* Select drive */
static void ata_select_drive(uint16_t base, uint8_t drive) {
    outb(base + 6, drive); /* Drive/head port */
    
    /* 400ns delay */
    for (int i = 0; i < 4; i++) {
        inb(base + 7);
    }
}

/* Identify drive */
int ata_identify(uint8_t drive) {
    if (drive >= 4) return -1;
    
    uint16_t base = (drive < 2) ? ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    uint8_t slave = (drive % 2) ? ATA_SLAVE : ATA_MASTER;
    
    /* Select drive */
    ata_select_drive(base, slave);
    
    /* Send IDENTIFY command */
    outb(base + 2, 0);  /* Sector count */
    outb(base + 3, 0);  /* LBA low */
    outb(base + 4, 0);  /* LBA mid */
    outb(base + 5, 0);  /* LBA high */
    outb(base + 7, ATA_CMD_IDENTIFY);
    
    /* Check if drive exists */
    uint8_t status = inb(base + 7);
    if (status == 0) {
        drives[drive].exists = 0;
        return -1; /* No drive */
    }
    
    /* Wait for data */
    if (ata_wait_drq(base) < 0) {
        drives[drive].exists = 0;
        return -1;
    }
    
    /* Read IDENTIFY data */
    inw_buffer(base, drives[drive].identify, 256);
    
    /* Extract size (words 60-61 for 28-bit LBA) */
    drives[drive].size = *(uint32_t*)&drives[drive].identify[60];
    drives[drive].exists = 1;
    
    return 0;
}

/* Read sectors */
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer) {
    if (drive >= 4 || !drives[drive].exists) return -1;
    if (sector_count == 0) return 0;
    
    uint16_t base = (drive < 2) ? ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    uint8_t slave = (drive % 2) ? ATA_SLAVE : ATA_MASTER;
    
    /* Select drive and set LBA mode */
    ata_select_drive(base, slave | 0x40 | ((lba >> 24) & 0x0F));
    
    if (ata_wait_ready(base) < 0) return -1;
    
    /* Send read command */
    outb(base + 2, sector_count);
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_READ_PIO);
    
    /* Read sectors */
    uint16_t* buf = (uint16_t*)buffer;
    for (int i = 0; i < sector_count; i++) {
        if (ata_wait_drq(base) < 0) return -1;
        inw_buffer(base, buf, 256); /* 256 words = 512 bytes */
        buf += 256;
    }
    
    return sector_count;
}

/* Write sectors */
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer) {
    if (drive >= 4 || !drives[drive].exists) return -1;
    if (sector_count == 0) return 0;
    
    uint16_t base = (drive < 2) ? ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    uint8_t slave = (drive % 2) ? ATA_SLAVE : ATA_MASTER;
    
    /* Select drive and set LBA mode */
    ata_select_drive(base, slave | 0x40 | ((lba >> 24) & 0x0F));
    
    if (ata_wait_ready(base) < 0) return -1;
    
    /* Send write command */
    outb(base + 2, sector_count);
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_WRITE_PIO);
    
    /* Write sectors */
    const uint16_t* buf = (const uint16_t*)buffer;
    for (int i = 0; i < sector_count; i++) {
        if (ata_wait_drq(base) < 0) return -1;
        outw_buffer(base, buf, 256);
        buf += 256;
    }
    
    /* Flush cache */
    outb(base + 7, ATA_CMD_CACHE_FLUSH);
    ata_wait_ready(base);
    
    return sector_count;
}

/* Get drive size */
uint32_t ata_get_size(uint8_t drive) {
    if (drive >= 4 || !drives[drive].exists) return 0;
    return drives[drive].size;
}

/* Driver operations */
static int ata_driver_init(void) {
    kprintf("[ATA] Initializing ATA driver...\n");
    
    /* Initialize drive info */
    for (int i = 0; i < 4; i++) {
        drives[i].exists = 0;
        drives[i].size = 0;
    }
    
    /* Detect drives */
    for (int i = 0; i < 4; i++) {
        if (ata_identify(i) == 0) {
            const char* channel = (i < 2) ? "Primary" : "Secondary";
            const char* position = (i % 2) ? "Slave" : "Master";
            kprintf("[ATA] %s %s: %u MB\n", channel, position,
                    (drives[i].size * 512) / (1024 * 1024));
        }
    }
    
    return 0;
}

static int ata_driver_open(uint32_t minor) {
    if (minor >= 4 || !drives[minor].exists) {
        return -1;
    }
    return 0;
}

static int ata_driver_close(uint32_t minor) {
    (void)minor;
    return 0;
}

static int ata_driver_read(uint32_t minor, void* buf, size_t count, uint32_t offset) {
    if (minor >= 4) return -1;
    
    uint32_t lba = offset / ATA_SECTOR_SIZE;
    uint32_t sector_count = (count + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
    
    if (sector_count > 255) sector_count = 255;
    
    return ata_read_sectors(minor, lba, sector_count, buf);
}

static int ata_driver_write(uint32_t minor, const void* buf, size_t count, uint32_t offset) {
    if (minor >= 4) return -1;
    
    uint32_t lba = offset / ATA_SECTOR_SIZE;
    uint32_t sector_count = (count + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
    
    if (sector_count > 255) sector_count = 255;
    
    return ata_write_sectors(minor, lba, sector_count, buf);
}

static int ata_driver_ioctl(uint32_t minor, uint32_t cmd, void* arg) {
    if (minor >= 4) return -1;
    
    switch (cmd) {
        case 0x1001: /* Get disk size */
            if (arg) {
                *(uint32_t*)arg = drives[minor].size;
                return 0;
            }
            break;
    }
    
    return -1;
}

static driver_ops_t ata_ops = {
    .init = ata_driver_init,
    .cleanup = NULL,
    .open = ata_driver_open,
    .close = ata_driver_close,
    .read = ata_driver_read,
    .write = ata_driver_write,
    .ioctl = ata_driver_ioctl
};

static driver_t ata_driver = {
    .name = "ata",
    .version = "1.0",
    .type = DRIVER_TYPE_BLOCK,
    .state = DRIVER_STATE_UNLOADED,
    .major = 3,
    .ops = &ata_ops,
    .private_data = NULL,
    .next = NULL
};

/* Initialize ATA driver */
int ata_init(void) {
    return driver_register(&ata_driver);
}