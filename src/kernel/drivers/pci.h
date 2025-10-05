/* pci.h - PCI bus driver */

#ifndef PCI_H
#define PCI_H

#include <stdint.h>

/* PCI device structure */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint32_t bar[6];
} pci_device_t;

/* Initialize PCI subsystem */
void pci_init(void);

/* Scan PCI bus */
void pci_scan(void);

/* Read PCI configuration space */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/* Write PCI configuration space */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/* Find PCI device by vendor/device ID */
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);

#endif /* PCI_H */