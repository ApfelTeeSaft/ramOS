/* libsys.h - ramOS System API
 * 
 * This is the userspace API for applications running on ramOS.
 * Applications link against this library to interact with the kernel.
 */

#ifndef LIBSYS_H
#define LIBSYS_H

#include <stdint.h>
#include <stddef.h>

/* System call numbers */
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

/* File open flags */
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0004
#define O_CREAT     0x0008
#define O_TRUNC     0x0010
#define O_APPEND    0x0020

/* File seek whence */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/* File types */
#define S_IFREG     0x8000
#define S_IFDIR     0x4000
#define S_IFCHR     0x2000
#define S_IFBLK     0x6000

/* Standard file descriptors */
#define STDIN       0
#define STDOUT      1
#define STDERR      2

/* Process states */
#define PROC_STATE_READY    0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_BLOCKED  2
#define PROC_STATE_ZOMBIE   3
#define PROC_STATE_DEAD     4

/* File stat structure */
typedef struct {
    uint32_t st_mode;
    uint32_t st_size;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
} stat_t;

/* Directory entry */
typedef struct {
    char name[256];
    uint32_t inode;
    uint32_t type;
} dirent_t;

/* Time structure */
typedef struct {
    uint32_t seconds;
    uint32_t milliseconds;
    uint32_t ticks;
} time_t;

/* Process info structure (NEW) */
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    char name[64];
    uint32_t memory_used;
    uint32_t cpu_time;
} proc_info_t;

/* Process API */
void sys_exit(int code);
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
int sys_wait(int* status);
int sys_getpid(void);
int sys_kill(int pid, int signal);  /* NEW */
int sys_getprocs(proc_info_t* procs, int max_count);  /* NEW */

/* File I/O API */
int sys_open(const char* path, int flags);
int sys_close(int fd);
int sys_read(int fd, void* buf, size_t count);
int sys_write(int fd, const void* buf, size_t count);
int sys_seek(int fd, int offset, int whence);
int sys_stat(const char* path, stat_t* buf);

/* Directory API */
int sys_readdir(int fd, dirent_t* entry);
int sys_mkdir(const char* path, uint32_t mode);
int sys_rmdir(const char* path);
int sys_unlink(const char* path);
int sys_getcwd(char* buf, size_t size);
int sys_chdir(const char* path);

/* Memory API */
void* sys_malloc(size_t size);
void sys_free(void* ptr);

/* Time API */
int sys_gettime(time_t* t);
void sys_sleep(uint32_t ms);

/* Filesystem API */
int sys_mount(const char* source, const char* target, const char* fstype);
int sys_umount(const char* target);

/* Driver API */
int sys_load_driver(const char* path);
int sys_ioctl(int fd, uint32_t request, void* arg);

/* Console I/O helpers */
void print(const char* str);
void println(const char* str);
int readln(char* buf, size_t max);
void printf(const char* fmt, ...);

/* String utilities */
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

/* Conversion utilities */
int atoi(const char* str);
char* itoa(int value, char* str, int base);

#endif /* LIBSYS_H */