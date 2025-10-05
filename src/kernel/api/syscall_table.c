/* syscall_table.c - System call dispatch table */

#include "syscalls.h"
#include "../proc/syscall.h"

/* System call handler function pointer type */
typedef int (*syscall_fn_t)(uint32_t, uint32_t, uint32_t);

/* System call table - maps syscall numbers to handlers */
syscall_fn_t syscall_table[] = {
    [SYS_EXIT]        = (syscall_fn_t)sys_exit,
    [SYS_WRITE]       = (syscall_fn_t)sys_write,
    [SYS_READ]        = (syscall_fn_t)sys_read,
    [SYS_OPEN]        = (syscall_fn_t)sys_open,
    [SYS_CLOSE]       = (syscall_fn_t)sys_close,
    [SYS_SEEK]        = (syscall_fn_t)sys_seek,
    [SYS_STAT]        = (syscall_fn_t)sys_stat,
    [SYS_GETPID]      = (syscall_fn_t)sys_getpid,
    [SYS_FORK]        = (syscall_fn_t)sys_fork,
    [SYS_EXEC]        = (syscall_fn_t)sys_exec,
    [SYS_WAIT]        = (syscall_fn_t)sys_wait,
    [SYS_MALLOC]      = (syscall_fn_t)sys_malloc,
    [SYS_FREE]        = (syscall_fn_t)sys_free,
    [SYS_GETTIME]     = (syscall_fn_t)sys_gettime,
    [SYS_SLEEP]       = (syscall_fn_t)sys_sleep,
    [SYS_READDIR]     = (syscall_fn_t)sys_readdir,
    [SYS_MKDIR]       = (syscall_fn_t)sys_mkdir,
    [SYS_RMDIR]       = (syscall_fn_t)sys_rmdir,
    [SYS_UNLINK]      = (syscall_fn_t)sys_unlink,
    [SYS_MOUNT]       = (syscall_fn_t)sys_mount,
    [SYS_UMOUNT]      = (syscall_fn_t)sys_umount,
    [SYS_LOAD_DRIVER] = (syscall_fn_t)sys_load_driver,
    [SYS_IOCTL]       = (syscall_fn_t)sys_ioctl,
    [SYS_GETCWD]      = (syscall_fn_t)sys_getcwd,
    [SYS_CHDIR]       = (syscall_fn_t)sys_chdir,
};

/* Number of system calls */
#define SYSCALL_COUNT (sizeof(syscall_table) / sizeof(syscall_fn_t))

/* System call dispatcher - called from syscall interrupt handler */
int syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    if (syscall_num >= SYSCALL_COUNT || syscall_table[syscall_num] == NULL) {
        return -1; /* Invalid syscall */
    }
    
    return syscall_table[syscall_num](arg1, arg2, arg3);
}