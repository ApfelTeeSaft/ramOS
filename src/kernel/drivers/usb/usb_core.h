/* usb_core.h - USB core functionality */

#ifndef USB_CORE_H
#define USB_CORE_H

#include <stdint.h>

/* USB device structure */
typedef struct {
    uint8_t address;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t protocol;
} usb_device_t;

/* USB endpoint types */
#define USB_ENDPOINT_CONTROL     0
#define USB_ENDPOINT_ISOCHRONOUS 1
#define USB_ENDPOINT_BULK        2
#define USB_ENDPOINT_INTERRUPT   3

/* USB request types */
#define USB_REQ_GET_STATUS        0
#define USB_REQ_CLEAR_FEATURE     1
#define USB_REQ_SET_FEATURE       3
#define USB_REQ_SET_ADDRESS       5
#define USB_REQ_GET_DESCRIPTOR    6
#define USB_REQ_SET_DESCRIPTOR    7
#define USB_REQ_GET_CONFIGURATION 8
#define USB_REQ_SET_CONFIGURATION 9

/* Initialize USB subsystem */
void usb_init(void);

/* Register USB device */
int usb_register_device(usb_device_t* device);

/* Send USB control transfer */
int usb_control_transfer(usb_device_t* device, uint8_t request_type,
                        uint8_t request, uint16_t value, uint16_t index,
                        void* data, uint16_t length);

#endif /* USB_CORE_H */