/* usb_core.c - USB core implementation (stub) */

#include "usb_core.h"
#include "../../core/console.h"

#define MAX_USB_DEVICES 16

static usb_device_t usb_devices[MAX_USB_DEVICES];
static int usb_device_count = 0;

/* Initialize USB subsystem */
void usb_init(void) {
    kprintf("[USB] Initializing USB subsystem...\n");
    usb_device_count = 0;
    
    /* TODO: Enumerate USB controllers and devices */
    kprintf("[USB] USB support is currently a stub\n");
}

/* Register USB device */
int usb_register_device(usb_device_t* device) {
    if (!device || usb_device_count >= MAX_USB_DEVICES) {
        return -1;
    }
    
    /* Copy device info */
    usb_devices[usb_device_count] = *device;
    usb_device_count++;
    
    kprintf("[USB] Registered device: VID=%04x PID=%04x\n", 
            device->vendor_id, device->product_id);
    
    return 0;
}

/* USB control transfer */
int usb_control_transfer(usb_device_t* device, uint8_t request_type,
                        uint8_t request, uint16_t value, uint16_t index,
                        void* data, uint16_t length) {
    (void)device;
    (void)request_type;
    (void)request;
    (void)value;
    (void)index;
    (void)data;
    (void)length;
    
    /* Stub implementation */
    return -1;
}