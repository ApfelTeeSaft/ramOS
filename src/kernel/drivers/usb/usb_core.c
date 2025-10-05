#include "usb_core.h"
#include "uhci.h"
#include "../../core/console.h"
#include "../../mm/heap.h"

#define MAX_USB_DEVICES 16
#define MAX_USB_CONTROLLERS 4

static usb_device_t usb_devices[MAX_USB_DEVICES];
static int usb_device_count = 0;

/* USB controller structure */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t io_base;
    uint8_t type;  /* 0=UHCI, 1=OHCI, 2=EHCI, 3=XHCI */
    void* controller_data;
} usb_controller_t;

static usb_controller_t usb_controllers[MAX_USB_CONTROLLERS];
static int controller_count = 0;

/* String copy helper */
static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

/* Initialize USB subsystem */
void usb_init(void) {
    kprintf("[USB] Initializing USB subsystem...\n");
    
    usb_device_count = 0;
    controller_count = 0;
    
    memset(usb_devices, 0, sizeof(usb_devices));
    memset(usb_controllers, 0, sizeof(usb_controllers));
    
    /* Enumerate USB controllers via PCI */
    usb_enumerate_controllers();
    
    /* Initialize detected controllers */
    for (int i = 0; i < controller_count; i++) {
        usb_init_controller(&usb_controllers[i]);
    }
    
    kprintf("[USB] Found %d USB controller(s)\n", controller_count);
    kprintf("[USB] USB subsystem initialized\n");
}

/* Enumerate USB controllers */
void usb_enumerate_controllers(void) {
    /* Use PCI to find USB controllers */
    extern void pci_scan(void);
    extern pci_device_t* pci_find_device(uint16_t, uint16_t);
    
    /* Look for common USB controller vendor/device IDs */
    /* Intel UHCI */
    pci_device_t* dev = pci_find_device(0x8086, 0x7020);
    if (dev && controller_count < MAX_USB_CONTROLLERS) {
        usb_controllers[controller_count].vendor_id = dev->vendor_id;
        usb_controllers[controller_count].device_id = dev->device_id;
        usb_controllers[controller_count].io_base = dev->bar[4] & 0xFFFC;
        usb_controllers[controller_count].type = 0;  /* UHCI */
        controller_count++;
    }
    
    /* Additional controller detection would go here */
}

/* Initialize USB controller */
int usb_init_controller(usb_controller_t* ctrl) {
    if (!ctrl) return -1;
    
    kprintf("[USB] Initializing controller: %04x:%04x\n", 
            ctrl->vendor_id, ctrl->device_id);
    
    switch (ctrl->type) {
        case 0:  /* UHCI */
            if (uhci_reset(ctrl->io_base) == 0) {
                uhci_start(ctrl->io_base);
                kprintf("[USB] UHCI controller started\n");
            }
            break;
        case 1:  /* OHCI */
            kprintf("[USB] OHCI not implemented\n");
            break;
        case 2:  /* EHCI */
            kprintf("[USB] EHCI not implemented\n");
            break;
        case 3:  /* XHCI */
            kprintf("[USB] XHCI not implemented\n");
            break;
    }
    
    return 0;
}

/* Register USB device */
int usb_register_device(usb_device_t* device) {
    if (!device || usb_device_count >= MAX_USB_DEVICES) {
        return -1;
    }
    
    /* Copy device info */
    memcpy(&usb_devices[usb_device_count], device, sizeof(usb_device_t));
    usb_device_count++;
    
    kprintf("[USB] Registered device: addr=%d VID=%04x PID=%04x class=%02x\n", 
            device->address, device->vendor_id, device->product_id, device->class_code);
    
    /* Load appropriate driver based on class */
    usb_load_driver_for_device(device);
    
    return 0;
}

/* Load driver for USB device */
void usb_load_driver_for_device(usb_device_t* device) {
    if (!device) return;
    
    kprintf("[USB] Looking for driver for class %02x:%02x\n", 
            device->class_code, device->subclass);
    
    switch (device->class_code) {
        case 0x03:  /* HID */
            kprintf("[USB] HID device detected\n");
            /* usb_hid_init(device); */
            break;
        case 0x08:  /* Mass Storage */
            kprintf("[USB] Mass storage device detected\n");
            /* usb_storage_init(device); */
            break;
        case 0x09:  /* Hub */
            kprintf("[USB] Hub detected\n");
            /* usb_hub_init(device); */
            break;
        default:
            kprintf("[USB] No driver for class %02x\n", device->class_code);
            break;
    }
}

/* USB control transfer */
int usb_control_transfer(usb_device_t* device, uint8_t request_type,
                        uint8_t request, uint16_t value, uint16_t index,
                        void* data, uint16_t length) {
    if (!device) return -1;
    
    kprintf("[USB] Control transfer: dev=%d req=%02x val=%04x len=%u\n",
            device->address, request, value, length);
    
    /* Build setup packet */
    typedef struct {
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } __attribute__((packed)) usb_setup_packet_t;
    
    usb_setup_packet_t setup;
    setup.bmRequestType = request_type;
    setup.bRequest = request;
    setup.wValue = value;
    setup.wIndex = index;
    setup.wLength = length;
    
    /* Submit control transfer to controller */
    /* This would interact with the UHCI/OHCI/EHCI/XHCI driver */
    
    /* Stub implementation */
    (void)data;
    
    return -1;
}

/* USB bulk transfer */
int usb_bulk_transfer(usb_device_t* device, uint8_t endpoint,
                      void* data, uint16_t length, int direction) {
    if (!device || !data) return -1;
    
    kprintf("[USB] Bulk transfer: dev=%d ep=%02x len=%u dir=%d\n",
            device->address, endpoint, length, direction);
    
    /* Submit bulk transfer to controller */
    /* Stub implementation */
    
    return -1;
}

/* USB interrupt transfer */
int usb_interrupt_transfer(usb_device_t* device, uint8_t endpoint,
                           void* data, uint16_t length, int direction) {
    if (!device || !data) return -1;
    
    kprintf("[USB] Interrupt transfer: dev=%d ep=%02x len=%u dir=%d\n",
            device->address, endpoint, length, direction);
    
    /* Submit interrupt transfer to controller */
    /* Stub implementation */
    
    return -1;
}

/* Get device descriptor */
int usb_get_device_descriptor(usb_device_t* device, void* descriptor) {
    if (!device || !descriptor) return -1;
    
    return usb_control_transfer(device, 0x80, USB_REQ_GET_DESCRIPTOR,
                               (0x01 << 8), 0, descriptor, 18);
}

/* Get configuration descriptor */
int usb_get_config_descriptor(usb_device_t* device, uint8_t config_index,
                              void* descriptor, uint16_t length) {
    if (!device || !descriptor) return -1;
    
    return usb_control_transfer(device, 0x80, USB_REQ_GET_DESCRIPTOR,
                               (0x02 << 8) | config_index, 0, 
                               descriptor, length);
}

/* Set device configuration */
int usb_set_configuration(usb_device_t* device, uint8_t config_value) {
    if (!device) return -1;
    
    return usb_control_transfer(device, 0x00, USB_REQ_SET_CONFIGURATION,
                               config_value, 0, NULL, 0);
}

/* Set device address */
int usb_set_address(usb_device_t* device, uint8_t address) {
    if (!device) return -1;
    
    int result = usb_control_transfer(device, 0x00, USB_REQ_SET_ADDRESS,
                                     address, 0, NULL, 0);
    
    if (result == 0) {
        device->address = address;
    }
    
    return result;
}

/* Get device status */
int usb_get_status(usb_device_t* device, uint16_t* status) {
    if (!device || !status) return -1;
    
    return usb_control_transfer(device, 0x80, USB_REQ_GET_STATUS,
                               0, 0, status, 2);
}

/* Clear feature */
int usb_clear_feature(usb_device_t* device, uint16_t feature) {
    if (!device) return -1;
    
    return usb_control_transfer(device, 0x00, USB_REQ_CLEAR_FEATURE,
                               feature, 0, NULL, 0);
}

/* Set feature */
int usb_set_feature(usb_device_t* device, uint16_t feature) {
    if (!device) return -1;
    
    return usb_control_transfer(device, 0x00, USB_REQ_SET_FEATURE,
                               feature, 0, NULL, 0);
}