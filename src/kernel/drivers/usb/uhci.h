/* uhci.h - USB UHCI (Universal Host Controller Interface) driver */

#ifndef UHCI_H
#define UHCI_H

#include <stdint.h>

/* UHCI PCI vendor/device IDs */
#define UHCI_VENDOR_INTEL  0x8086
#define UHCI_DEVICE_PIIX3  0x7020
#define UHCI_DEVICE_PIIX4  0x7112

/* UHCI registers (I/O space offsets) */
#define UHCI_USBCMD     0x00  /* USB Command */
#define UHCI_USBSTS     0x02  /* USB Status */
#define UHCI_USBINTR    0x04  /* USB Interrupt Enable */
#define UHCI_FRNUM      0x06  /* Frame Number */
#define UHCI_FRBASEADD  0x08  /* Frame List Base Address */
#define UHCI_SOFMOD     0x0C  /* Start of Frame Modify */
#define UHCI_PORTSC1    0x10  /* Port 1 Status/Control */
#define UHCI_PORTSC2    0x12  /* Port 2 Status/Control */

/* UHCI command register bits */
#define UHCI_CMD_RS     0x01  /* Run/Stop */
#define UHCI_CMD_HCRESET 0x02 /* Host Controller Reset */
#define UHCI_CMD_GRESET  0x04 /* Global Reset */
#define UHCI_CMD_SWDBG   0x10 /* Software Debug */
#define UHCI_CMD_CF      0x40 /* Configure Flag */
#define UHCI_CMD_MAXP    0x80 /* Max Packet (0=32, 1=64) */

/* UHCI status register bits */
#define UHCI_STS_USBINT  0x01 /* USB Interrupt */
#define UHCI_STS_ERROR   0x02 /* USB Error Interrupt */
#define UHCI_STS_RD      0x04 /* Resume Detect */
#define UHCI_STS_HSE     0x08 /* Host System Error */
#define UHCI_STS_HCPE    0x10 /* Host Controller Process Error */
#define UHCI_STS_HCH     0x20 /* HC Halted */

/* Initialize UHCI controller */
void uhci_init(void);

/* Reset UHCI controller */
int uhci_reset(uint16_t io_base);

/* Start UHCI controller */
int uhci_start(uint16_t io_base);

/* Stop UHCI controller */
int uhci_stop(uint16_t io_base);

#endif /* UHCI_H */