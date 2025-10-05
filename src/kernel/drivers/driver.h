/* driver.h - Driver Framework for ramOS */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

/* Driver types */
typedef enum {
    DRIVER_TYPE_BLOCK,      /* Block device (disk) */
    DRIVER_TYPE_CHAR,       /* Character device (serial, keyboard) */
    DRIVER_TYPE_NET,        /* Network device */
    DRIVER_TYPE_USB,        /* USB device */
    DRIVER_TYPE_OTHER       /* Other */
} driver_type_t;

/* Driver state */
typedef enum {
    DRIVER_STATE_UNLOADED,
    DRIVER_STATE_LOADING,
    DRIVER_STATE_LOADED,
    DRIVER_STATE_ERROR
} driver_state_t;

/* Driver operations */
typedef struct driver_ops {
    int (*init)(void);
    int (*cleanup)(void);
    int (*open)(uint32_t minor);
    int (*close)(uint32_t minor);
    int (*read)(uint32_t minor, void* buf, size_t count, uint32_t offset);
    int (*write)(uint32_t minor, const void* buf, size_t count, uint32_t offset);
    int (*ioctl)(uint32_t minor, uint32_t cmd, void* arg);
} driver_ops_t;

/* Driver structure */
typedef struct driver {
    char name[64];
    char version[16];
    driver_type_t type;
    driver_state_t state;
    uint32_t major;
    driver_ops_t* ops;
    void* private_data;
    struct driver* next;
} driver_t;

/* Driver framework API */
int driver_init(void);
int driver_register(driver_t* driver);
int driver_unregister(driver_t* driver);
driver_t* driver_find(const char* name);
driver_t* driver_find_by_major(uint32_t major);
int driver_load_from_file(const char* path);

/* Device file operations */
int dev_open(const char* name, int flags);
int dev_close(int fd);
int dev_read(int fd, void* buf, size_t count);
int dev_write(int fd, const void* buf, size_t count);
int dev_ioctl(int fd, uint32_t cmd, void* arg);

#endif /* DRIVER_H */