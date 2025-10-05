/* driver.c - Driver framework implementation */

#include "driver.h"
#include "../core/console.h"
#include "../mm/heap.h"
#include "../fs/vfs.h"

static driver_t* driver_list = NULL;
static uint32_t next_major = 1;

/* String utilities */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Initialize driver framework */
int driver_init(void) {
    kprintf("[DRV] Initializing driver framework...\n");
    driver_list = NULL;
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
    
    /* TODO: Implement dynamic driver loading */
    /* Would need to:
     * 1. Load driver binary
     * 2. Parse and relocate
     * 3. Call driver initialization
     * 4. Register driver
     */
    
    return -1; /* Not implemented */
}

/* Device file operations */
int dev_open(const char* name, int flags) {
    (void)name;
    (void)flags;
    /* TODO: Parse /dev/name and route to appropriate driver */
    return -1;
}

int dev_close(int fd) {
    (void)fd;
    return -1;
}

int dev_read(int fd, void* buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

int dev_write(int fd, const void* buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

int dev_ioctl(int fd, uint32_t cmd, void* arg) {
    (void)fd;
    (void)cmd;
    (void)arg;
    return -1;
}