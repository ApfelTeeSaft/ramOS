/* syscalls.h - System call implementations (kernel side) */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

/* System call numbers (must match libsys.h) */
#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2
#define SYS_OPEN        3
#define SYS_CLOSE       4
#define SYS_SEEK        5
#define SYS_STAT        6
#define SYS_GETPID      7
#define SYS_FORK        8
#define SYS_EXEC        9
#define SYS_WAIT        10
#define SYS_MALLOC      11
#define SYS_FREE        12
#define SYS_GETTIME     13
#define SYS_SLEEP       14
#define SYS_READDIR     15
#define SYS_MKDIR       16
#define SYS_RMDIR       17
#define SYS_UNLINK      18
#define SYS_MOUNT       19
#define SYS_UMOUNT      20
#define SYS_LOAD_DRIVER 21
#define SYS_IOCTL       22
#define SYS_GETCWD      23
#define SYS_CHDIR       24
#define SYS_KILL        25  /* NEW */
#define SYS_GETPROCS    26  /* NEW */

/* System call implementations */
int sys_exit(int code);
int sys_write(int fd, const void* buf, size_t count);
int sys_read(int fd, void* buf, size_t count);
int sys_open(const char* path, int flags);
int sys_close(int fd);
int sys_seek(int fd, int offset, int whence);
int sys_stat(const char* path, void* statbuf);
int sys_getpid(void);
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
int sys_wait(int* status);
void* sys_malloc(size_t size);
int sys_free(void* ptr);
int sys_gettime(void* timebuf);
int sys_sleep(uint32_t ms);
int sys_readdir(int fd, void* entry);
int sys_mkdir(const char* path, uint32_t mode);
int sys_rmdir(const char* path);
int sys_unlink(const char* path);
int sys_mount(const char* source, const char* target, const char* fstype);
int sys_umount(const char* target);
int sys_load_driver(const char* path);
int sys_ioctl(int fd, uint32_t request, void* arg);
int sys_getcwd(char* buf, size_t size);
int sys_chdir(const char* path);
int sys_kill(int pid, int signal);         /* NEW */
int sys_getprocs(void* procs, int max_count);  /* NEW */

#endif /* SYSCALLS_H */