/* driver.c - Driver framework with complete device I/O */

#include "driver.h"
#include "../core/console.h"
#include "../mm/heap.h"
#include "../fs/vfs.h"

#define MAX_OPEN_DEVICES 64

static driver_t* driver_list = NULL;
static uint32_t next_major = 1;

/* Device file descriptor tracking */
typedef struct {
    driver_t* driver;
    uint32_t minor;
    int flags;
    uint32_t position;
} device_fd_t;

static device_fd_t device_fds[MAX_OPEN_DEVICES];

/* String utilities */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int atoi(const char* str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

/* Initialize driver framework */
int driver_init(void) {
    kprintf("[DRV] Initializing driver framework...\n");
    driver_list = NULL;
    
    /* Clear device FD table */
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        device_fds[i].driver = NULL;
        device_fds[i].minor = 0;
        device_fds[i].flags = 0;
        device_fds[i].position = 0;
    }
    
    return 0;
}

/* Register driver */
int driver_register(driver_t* driver) {
    if (!driver) return -1;
    
    /* Assign major number if not set */
    if (driver->major == 0) {
        driver->major = next_major++;
    }
    
    /* Initialize driver */
    if (driver->ops && driver->ops->init) {
        if (driver->ops->init() < 0) {
            driver->state = DRIVER_STATE_ERROR;
            return -1;
        }
    }
    
    driver->state = DRIVER_STATE_LOADED;
    
    /* Add to driver list */
    driver->next = driver_list;
    driver_list = driver;
    
    kprintf("[DRV] Registered driver: %s (major %u)\n", driver->name, driver->major);
    
    return 0;
}

/* Unregister driver */
int driver_unregister(driver_t* driver) {
    if (!driver) return -1;
    
    /* Cleanup driver */
    if (driver->ops && driver->ops->cleanup) {
        driver->ops->cleanup();
    }
    
    /* Remove from list */
    driver_t** current = &driver_list;
    while (*current) {
        if (*current == driver) {
            *current = driver->next;
            break;
        }
        current = &(*current)->next;
    }
    
    driver->state = DRIVER_STATE_UNLOADED;
    
    return 0;
}

/* Find driver by name */
driver_t* driver_find(const char* name) {
    for (driver_t* drv = driver_list; drv != NULL; drv = drv->next) {
        if (strcmp(drv->name, name) == 0) {
            return drv;
        }
    }
    return NULL;
}

/* Find driver by major number */
driver_t* driver_find_by_major(uint32_t major) {
    for (driver_t* drv = driver_list; drv != NULL; drv = drv->next) {
        if (drv->major == major) {
            return drv;
        }
    }
    return NULL;
}

/* Load driver from file */
int driver_load_from_file(const char* path) {
    kprintf("[DRV] Loading driver from: %s\n", path);
    
    /* TODO: Implement dynamic driver loading
     * Steps needed:
     * 1. Open and read driver binary
     * 2. Verify driver signature/magic
     * 3. Parse and relocate ELF sections
     * 4. Resolve symbols against kernel
     * 5. Call driver initialization
     * 6. Register driver
     */
    
    kprintf("[DRV] Dynamic driver loading not yet implemented\n");
    return -1;
}

/* Parse device name to extract driver and minor number */
static int parse_device_name(const char* name, char* driver_name, uint32_t* minor) {
    if (!name || !driver_name || !minor) return -1;
    
    /* Expected format: /dev/drivername[minor] */
    /* Examples: /dev/sda, /dev/sda1, /dev/tty0 */
    
    /* Skip /dev/ prefix */
    if (strncmp(name, "/dev/", 5) == 0) {
        name += 5;
    }
    
    /* Copy driver name and extract minor number */
    int i = 0;
    while (*name && (*name < '0' || *name > '9') && i < 63) {
        driver_name[i++] = *name++;
    }
    driver_name[i] = '\0';
    
    /* Parse minor number if present */
    *minor = 0;
    if (*name >= '0' && *name <= '9') {
        *minor = atoi(name);
    }
    
    return 0;
}

/* Allocate device file descriptor */
static int alloc_device_fd(void) {
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        if (device_fds[i].driver == NULL) {
            return i;
        }
    }
    return -1;
}

/* Device file operations */
int dev_open(const char* name, int flags) {
    if (!name) return -1;
    
    char driver_name[64];
    uint32_t minor;
    
    /* Parse device name */
    if (parse_device_name(name, driver_name, &minor) < 0) {
        kprintf("[DRV] Invalid device name: %s\n", name);
        return -1;
    }
    
    /* Find driver */
    driver_t* driver = driver_find(driver_name);
    if (!driver) {
        kprintf("[DRV] Driver not found: %s\n", driver_name);
        return -1;
    }
    
    /* Check if driver supports open */
    if (!driver->ops || !driver->ops->open) {
        kprintf("[DRV] Driver does not support open: %s\n", driver_name);
        return -1;
    }
    
    /* Open device */
    if (driver->ops->open(minor) < 0) {
        kprintf("[DRV] Failed to open device: %s%u\n", driver_name, minor);
        return -1;
    }
    
    /* Allocate file descriptor */
    int fd = alloc_device_fd();
    if (fd < 0) {
        kprintf("[DRV] No free device file descriptors\n");
        driver->ops->close(minor);
        return -1;
    }
    
    /* Set up device FD */
    device_fds[fd].driver = driver;
    device_fds[fd].minor = minor;
    device_fds[fd].flags = flags;
    device_fds[fd].position = 0;
    
    kprintf("[DRV] Opened device: %s%u (fd=%d)\n", driver_name, minor, fd);
    
    return fd;
}

int dev_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_DEVICES || !device_fds[fd].driver) {
        return -1;
    }
    
    driver_t* driver = device_fds[fd].driver;
    uint32_t minor = device_fds[fd].minor;
    
    /* Close device */
    if (driver->ops && driver->ops->close) {
        driver->ops->close(minor);
    }
    
    /* Clear FD */
    device_fds[fd].driver = NULL;
    device_fds[fd].minor = 0;
    device_fds[fd].flags = 0;
    device_fds[fd].position = 0;
    
    return 0;
}

int dev_read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_DEVICES || !device_fds[fd].driver) {
        return -1;
    }
    
    driver_t* driver = device_fds[fd].driver;
    uint32_t minor = device_fds[fd].minor;
    
    if (!driver->ops || !driver->ops->read) {
        return -1;
    }
    
    int bytes_read = driver->ops->read(minor, buf, count, device_fds[fd].position);
    
    if (bytes_read > 0) {
        device_fds[fd].position += bytes_read;
    }
    
    return bytes_read;
}

int dev_write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_DEVICES || !device_fds[fd].driver) {
        return -1;
    }
    
    driver_t* driver = device_fds[fd].driver;
    uint32_t minor = device_fds[fd].minor;
    
    if (!driver->ops || !driver->ops->write) {
        return -1;
    }
    
    int bytes_written = driver->ops->write(minor, buf, count, device_fds[fd].position);
    
    if (bytes_written > 0) {
        device_fds[fd].position += bytes_written;
    }
    
    return bytes_written;
}

int dev_ioctl(int fd, uint32_t cmd, void* arg) {
    if (fd < 0 || fd >= MAX_OPEN_DEVICES || !device_fds[fd].driver) {
        return -1;
    }
    
    driver_t* driver = device_fds[fd].driver;
    uint32_t minor = device_fds[fd].minor;
    
    if (!driver->ops || !driver->ops->ioctl) {
        return -1;
    }
    
    return driver->ops->ioctl(minor, cmd, arg);
}

/* Seek in device (for block devices) */
int dev_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_DEVICES || !device_fds[fd].driver) {
        return -1;
    }
    
    uint32_t new_pos;
    
    switch (whence) {
        case 0: /* SEEK_SET */
            new_pos = offset;
            break;
        case 1: /* SEEK_CUR */
            new_pos = device_fds[fd].position + offset;
            break;
        case 2: /* SEEK_END */
            /* Would need device size */
            return -1;
        default:
            return -1;
    }
    
    device_fds[fd].position = new_pos;
    return new_pos;
}