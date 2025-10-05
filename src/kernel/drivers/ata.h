/* ata.h - ATA/IDE Disk Driver for VirtualBox */

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* ATA I/O ports (primary channel) */
#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_SECCOUNT   0x1F2
#define ATA_PRIMARY_LBA_LO     0x1F3
#define ATA_PRIMARY_LBA_MID    0x1F4
#define ATA_PRIMARY_LBA_HI     0x1F5
#define ATA_PRIMARY_DRIVE      0x1F6
#define ATA_PRIMARY_STATUS     0x1F7
#define ATA_PRIMARY_COMMAND    0x1F7

/* ATA I/O ports (secondary channel) */
#define ATA_SECONDARY_DATA     0x170
#define ATA_SECONDARY_ERROR    0x171
#define ATA_SECONDARY_SECCOUNT 0x172
#define ATA_SECONDARY_LBA_LO   0x173
#define ATA_SECONDARY_LBA_MID  0x174
#define ATA_SECONDARY_LBA_HI   0x175
#define ATA_SECONDARY_DRIVE    0x176
#define ATA_SECONDARY_STATUS   0x177
#define ATA_SECONDARY_COMMAND  0x177

/* ATA commands */
#define ATA_CMD_READ_PIO       0x20
#define ATA_CMD_READ_PIO_EXT   0x24
#define ATA_CMD_WRITE_PIO      0x30
#define ATA_CMD_WRITE_PIO_EXT  0x34
#define ATA_CMD_CACHE_FLUSH    0xE7
#define ATA_CMD_IDENTIFY       0xEC

/* ATA status flags */
#define ATA_SR_BSY   0x80    /* Busy */
#define ATA_SR_DRDY  0x40    /* Drive ready */
#define ATA_SR_DF    0x20    /* Drive fault */
#define ATA_SR_DSC   0x10    /* Drive seek complete */
#define ATA_SR_DRQ   0x08    /* Data request ready */
#define ATA_SR_CORR  0x04    /* Corrected data */
#define ATA_SR_IDX   0x02    /* Index */
#define ATA_SR_ERR   0x01    /* Error */

/* ATA drive selection */
#define ATA_MASTER   0xA0
#define ATA_SLAVE    0xB0

/* Sector size */
#define ATA_SECTOR_SIZE 512

/* Initialize ATA driver */
int ata_init(void);

/* Read sectors from disk */
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer);

/* Write sectors to disk */
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer);

/* Get drive information */
int ata_identify(uint8_t drive);

/* Get drive size in sectors */
uint32_t ata_get_size(uint8_t drive);

#endif /* ATA_H */