/* pci.c - PCI bus driver implementation */

#include "pci.h"
#include "../core/console.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define MAX_PCI_DEVICES 32

static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

/* Port I/O */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Read PCI configuration space */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | 
                       ((uint32_t)device << 11) | ((uint32_t)function << 8) | 
                       (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

/* Write PCI configuration space */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | 
                       ((uint32_t)device << 11) | ((uint32_t)function << 8) | 
                       (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

/* Check if device exists */
static int pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t value = pci_config_read(bus, device, function, 0);
    uint16_t vendor_id = value & 0xFFFF;
    
    return (vendor_id != 0xFFFF);
}

/* Scan PCI bus */
void pci_scan(void) {
    kprintf("[PCI] Scanning PCI bus...\n");
    
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            for (int function = 0; function < 8; function++) {
                if (!pci_device_exists(bus, device, function)) {
                    if (function == 0) break; /* No more functions */
                    continue;
                }
                
                if (pci_device_count >= MAX_PCI_DEVICES) {
                    kprintf("[PCI] Warning: Too many PCI devices\n");
                    return;
                }
                
                pci_device_t* dev = &pci_devices[pci_device_count++];
                
                uint32_t reg0 = pci_config_read(bus, device, function, 0);
                uint32_t reg2 = pci_config_read(bus, device, function, 8);
                
                dev->vendor_id = reg0 & 0xFFFF;
                dev->device_id = (reg0 >> 16) & 0xFFFF;
                dev->bus = bus;
                dev->device = device;
                dev->function = function;
                dev->class_code = (reg2 >> 24) & 0xFF;
                dev->subclass = (reg2 >> 16) & 0xFF;
                dev->prog_if = (reg2 >> 8) & 0xFF;
                dev->revision = reg2 & 0xFF;
                
                /* Read BARs */
                for (int i = 0; i < 6; i++) {
                    dev->bar[i] = pci_config_read(bus, device, function, 0x10 + i * 4);
                }
                
                kprintf("[PCI] %02x:%02x.%x - %04x:%04x (Class: %02x:%02x)\n",
                       bus, device, function, dev->vendor_id, dev->device_id,
                       dev->class_code, dev->subclass);
            }
        }
    }
    
    kprintf("[PCI] Found %d devices\n", pci_device_count);
}

/* Initialize PCI */
void pci_init(void) {
    kprintf("[PCI] Initializing PCI subsystem...\n");
    pci_device_count = 0;
    pci_scan();
}

/* Find device */
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && 
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}