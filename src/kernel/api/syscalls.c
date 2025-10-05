/* syscalls.c - System call implementations */

#include "syscalls.h"
#include "../proc/process.h"
#include "../mm/heap.h"
#include "../fs/vfs.h"
#include "../drivers/driver.h"
#include "../core/timer.h"
#include "../core/console.h"

/* String utilities */
static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

/* Exit current process */
int sys_exit(int code) {
    process_t* current = process_get_current();
    if (current) {
        current->exit_code = code;
        process_exit(current);
    }
    return 0;
}

/* Write to file descriptor */
int sys_write(int fd, const void* buf, size_t count) {
    if (fd == 1 || fd == 2) { /* STDOUT or STDERR */
        const char* str = (const char*)buf;
        for (size_t i = 0; i < count; i++) {
            console_putchar(str[i]);
        }
        return count;
    }
    
    return vfs_write(fd, buf, count);
}

/* Read from file descriptor */
int sys_read(int fd, void* buf, size_t count) {
    if (fd == 0) { /* STDIN */
        /* Use keyboard for input */
        extern void keyboard_read_line(char* buffer, size_t max_len);
        keyboard_read_line((char*)buf, count);
        return strlen((char*)buf);
    }
    
    return vfs_read(fd, buf, count);
}

/* Open file */
int sys_open(const char* path, int flags) {
    return vfs_open(path, flags);
}

/* Close file */
int sys_close(int fd) {
    return vfs_close(fd);
}

/* Seek in file */
int sys_seek(int fd, int offset, int whence) {
    return vfs_seek(fd, offset, whence);
}

/* Get file statistics */
int sys_stat(const char* path, void* statbuf) {
    return vfs_stat(path, statbuf);
}

/* Get process ID */
int sys_getpid(void) {
    process_t* current = process_get_current();
    return current ? current->pid : 0;
}

/* Fork process */
int sys_fork(void) {
    process_t* current = process_get_current();
    if (!current) return -1;
    
    process_t* child = process_fork(current);
    if (!child) return -1;
    
    return child->pid;
}

/* Execute program */
int sys_exec(const char* path, char* const argv[]) {
    process_t* current = process_get_current();
    if (!current) return -1;
    
    return process_exec(current, path, argv);
}

/* Wait for child process */
int sys_wait(int* status) {
    process_t* current = process_get_current();
    if (!current) return -1;
    
    return process_wait(current, status);
}

/* Kill process - NEW */
int sys_kill(int pid, int signal) {
    /* Don't allow killing kernel process */
    if (pid == 0) {
        return -1;
    }
    
    /* Check if process exists */
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        return -1;
    }
    
    /* Check permissions (for now, any process can kill any other) */
    /* In a real OS, would check if current process has permission */
    
    return process_kill(pid, signal);
}

/* Get process list - NEW */
int sys_getprocs(void* procs_buf, int max_count) {
    if (!procs_buf || max_count <= 0) return -1;
    
    /* Process info structure from libsys.h */
    typedef struct {
        uint32_t pid;
        uint32_t ppid;
        uint32_t state;
        char name[64];
        uint32_t memory_used;
        uint32_t cpu_time;
    } proc_info_t;
    
    proc_info_t* procs = (proc_info_t*)procs_buf;
    int count = 0;
    
    /* Iterate through process list */
    extern process_t* process_list;  /* Need to expose this */
    
    /* For now, use process_get_by_pid to iterate */
    for (int pid = 0; pid < 256 && count < max_count; pid++) {
        process_t* proc = process_get_by_pid(pid);
        if (proc) {
            procs[count].pid = proc->pid;
            procs[count].ppid = proc->parent_pid;
            procs[count].state = proc->state;
            strncpy(procs[count].name, proc->name, 63);
            procs[count].name[63] = '\0';
            procs[count].memory_used = 0;  /* TODO: Calculate actual memory usage */
            procs[count].cpu_time = timer_get_ticks() - proc->start_time;
            count++;
        }
    }
    
    return count;
}

/* Allocate memory */
void* sys_malloc(size_t size) {
    return kmalloc(size);
}

/* Free memory */
int sys_free(void* ptr) {
    kfree(ptr);
    return 0;
}

/* Get current time */
int sys_gettime(void* timebuf) {
    if (!timebuf) return -1;
    
    typedef struct {
        uint32_t seconds;
        uint32_t milliseconds;
        uint32_t ticks;
    } time_t;
    
    time_t* t = (time_t*)timebuf;
    uint32_t ms = timer_get_uptime_ms();
    
    t->ticks = timer_get_ticks();
    t->milliseconds = ms;
    t->seconds = ms / 1000;
    
    return 0;
}

/* Sleep for milliseconds */
int sys_sleep(uint32_t ms) {
    uint32_t start = timer_get_uptime_ms();
    uint32_t end = start + ms;
    
    while (timer_get_uptime_ms() < end) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

/* Read directory entry */
int sys_readdir(int fd, void* entry) {
    return vfs_readdir(fd, entry);
}

/* Create directory */
int sys_mkdir(const char* path, uint32_t mode) {
    return vfs_mkdir(path, mode);
}

/* Remove directory */
int sys_rmdir(const char* path) {
    return vfs_rmdir(path);
}

/* Remove file */
int sys_unlink(const char* path) {
    return vfs_unlink(path);
}

/* Mount filesystem */
int sys_mount(const char* source, const char* target, const char* fstype) {
    return vfs_mount(source, target, fstype);
}

/* Unmount filesystem */
int sys_umount(const char* target) {
    return vfs_umount(target);
}

/* Load driver */
int sys_load_driver(const char* path) {
    return driver_load_from_file(path);
}

/* Device I/O control */
int sys_ioctl(int fd, uint32_t request, void* arg) {
    return dev_ioctl(fd, request, arg);
}

/* Get current working directory */
int sys_getcwd(char* buf, size_t size) {
    process_t* current = process_get_current();
    if (!current || !buf) return -1;
    
    size_t len = strlen(current->cwd);
    if (len >= size) return -1;
    
    strcpy(buf, current->cwd);
    return len;
}

/* Change directory */
int sys_chdir(const char* path) {
    process_t* current = process_get_current();
    if (!current || !path) return -1;
    
    /* Verify directory exists */
    vfs_node_t* node = vfs_finddir(path);
    if (!node || !(node->flags & VFS_DIRECTORY)) {
        return -1;
    }
    
    strncpy(current->cwd, path, sizeof(current->cwd) - 1);
    current->cwd[sizeof(current->cwd) - 1] = '\0';
    
    return 0;
}