/* uhci.c - USB UHCI controller driver implementation (stub) */

#include "uhci.h"
#include "usb_core.h"
#include "../pci.h"
#include "../../core/console.h"

/* Port I/O */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Initialize UHCI controller */
void uhci_init(void) {
    kprintf("[UHCI] Initializing UHCI controller...\n");
    
    /* Find UHCI controller via PCI */
    pci_device_t* uhci_dev = pci_find_device(UHCI_VENDOR_INTEL, UHCI_DEVICE_PIIX4);
    
    if (!uhci_dev) {
        /* Try PIIX3 */
        uhci_dev = pci_find_device(UHCI_VENDOR_INTEL, UHCI_DEVICE_PIIX3);
    }
    
    if (!uhci_dev) {
        kprintf("[UHCI] No UHCI controller found\n");
        return;
    }
    
    kprintf("[UHCI] Found UHCI controller: %04x:%04x\n", 
            uhci_dev->vendor_id, uhci_dev->device_id);
    
    /* Get I/O base address from BAR4 */
    uint32_t io_base = uhci_dev->bar[4] & 0xFFFFFFFC;
    
    if (io_base == 0) {
        kprintf("[UHCI] Invalid I/O base address\n");
        return;
    }
    
    kprintf("[UHCI] I/O Base: %x\n", io_base);
    
    /* Reset controller */
    if (uhci_reset(io_base) < 0) {
        kprintf("[UHCI] Failed to reset controller\n");
        return;
    }
    
    /* Initialize USB core */
    usb_init();
    
    kprintf("[UHCI] UHCI controller initialized (stub)\n");
}

/* Reset UHCI controller */
int uhci_reset(uint16_t io_base) {
    kprintf("[UHCI] Resetting controller...\n");
    
    /* Send reset command */
    outw(io_base + UHCI_USBCMD, UHCI_CMD_HCRESET);
    
    /* Wait for reset to complete */
    int timeout = 1000;
    while (timeout-- > 0) {
        if (!(inw(io_base + UHCI_USBCMD) & UHCI_CMD_HCRESET)) {
            break;
        }
        /* Small delay */
        for (volatile int i = 0; i < 1000; i++);
    }
    
    if (timeout <= 0) {
        return -1;
    }
    
    kprintf("[UHCI] Reset complete\n");
    return 0;
}

/* Start UHCI controller */
int uhci_start(uint16_t io_base) {
    kprintf("[UHCI] Starting controller...\n");
    
    /* Set Run/Stop bit */
    uint16_t cmd = inw(io_base + UHCI_USBCMD);
    cmd |= UHCI_CMD_RS;
    outw(io_base + UHCI_USBCMD, cmd);
    
    /* Wait for controller to start */
    int timeout = 1000;
    while (timeout-- > 0) {
        if (!(inw(io_base + UHCI_USBSTS) & UHCI_STS_HCH)) {
            break;
        }
        for (volatile int i = 0; i < 1000; i++);
    }
    
    if (timeout <= 0) {
        return -1;
    }
    
    kprintf("[UHCI] Controller started\n");
    return 0;
}

/* Stop UHCI controller */
int uhci_stop(uint16_t io_base) {
    kprintf("[UHCI] Stopping controller...\n");
    
    /* Clear Run/Stop bit */
    uint16_t cmd = inw(io_base + UHCI_USBCMD);
    cmd &= ~UHCI_CMD_RS;
    outw(io_base + UHCI_USBCMD, cmd);
    
    /* Wait for controller to halt */
    int timeout = 1000;
    while (timeout-- > 0) {
        if (inw(io_base + UHCI_USBSTS) & UHCI_STS_HCH) {
            break;
        }
        for (volatile int i = 0; i < 1000; i++);
    }
    
    if (timeout <= 0) {
        return -1;
    }
    
    kprintf("[UHCI] Controller stopped\n");
    return 0;
}