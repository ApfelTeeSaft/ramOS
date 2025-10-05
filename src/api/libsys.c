/* libsys.c - ramOS System API Implementation */

#include "libsys.h"

/* Low-level system call invocation */
static inline int syscall0(int num) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

static inline int syscall1(int num, uint32_t arg1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline int syscall2(int num, uint32_t arg1, uint32_t arg2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

static inline int syscall3(int num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/* Process API */
void sys_exit(int code) {
    syscall1(SYS_EXIT, code);
    while(1); /* Should never reach here */
}

int sys_fork(void) {
    return syscall0(SYS_FORK);
}

int sys_exec(const char* path, char* const argv[]) {
    return syscall2(SYS_EXEC, (uint32_t)path, (uint32_t)argv);
}

int sys_wait(int* status) {
    return syscall1(SYS_WAIT, (uint32_t)status);
}

int sys_getpid(void) {
    return syscall0(SYS_GETPID);
}

/* File I/O API */
int sys_open(const char* path, int flags) {
    return syscall2(SYS_OPEN, (uint32_t)path, flags);
}

int sys_close(int fd) {
    return syscall1(SYS_CLOSE, fd);
}

int sys_read(int fd, void* buf, size_t count) {
    return syscall3(SYS_READ, fd, (uint32_t)buf, count);
}

int sys_write(int fd, const void* buf, size_t count) {
    return syscall3(SYS_WRITE, fd, (uint32_t)buf, count);
}

int sys_seek(int fd, int offset, int whence) {
    return syscall3(SYS_SEEK, fd, offset, whence);
}

int sys_stat(const char* path, stat_t* buf) {
    return syscall2(SYS_STAT, (uint32_t)path, (uint32_t)buf);
}

/* Directory API */
int sys_readdir(int fd, dirent_t* entry) {
    return syscall2(SYS_READDIR, fd, (uint32_t)entry);
}

int sys_mkdir(const char* path, uint32_t mode) {
    return syscall2(SYS_MKDIR, (uint32_t)path, mode);
}

int sys_rmdir(const char* path) {
    return syscall1(SYS_RMDIR, (uint32_t)path);
}

int sys_unlink(const char* path) {
    return syscall1(SYS_UNLINK, (uint32_t)path);
}

int sys_getcwd(char* buf, size_t size) {
    return syscall2(SYS_GETCWD, (uint32_t)buf, size);
}

int sys_chdir(const char* path) {
    return syscall1(SYS_CHDIR, (uint32_t)path);
}

/* Memory API */
void* sys_malloc(size_t size) {
    return (void*)syscall1(SYS_MALLOC, size);
}

void sys_free(void* ptr) {
    syscall1(SYS_FREE, (uint32_t)ptr);
}

/* Time API */
int sys_gettime(time_t* t) {
    return syscall1(SYS_GETTIME, (uint32_t)t);
}

void sys_sleep(uint32_t ms) {
    syscall1(SYS_SLEEP, ms);
}

/* Filesystem API */
int sys_mount(const char* source, const char* target, const char* fstype) {
    return syscall3(SYS_MOUNT, (uint32_t)source, (uint32_t)target, (uint32_t)fstype);
}

int sys_umount(const char* target) {
    return syscall1(SYS_UMOUNT, (uint32_t)target);
}

/* Driver API */
int sys_load_driver(const char* path) {
    return syscall1(SYS_LOAD_DRIVER, (uint32_t)path);
}

int sys_ioctl(int fd, uint32_t request, void* arg) {
    return syscall3(SYS_IOCTL, fd, request, (uint32_t)arg);
}

/* Console I/O helpers */
void print(const char* str) {
    sys_write(STDOUT, str, strlen(str));
}

void println(const char* str) {
    print(str);
    print("\n");
}

int readln(char* buf, size_t max) {
    size_t pos = 0;
    char c;
    
    while (pos < max - 1) {
        if (sys_read(STDIN, &c, 1) <= 0) break;
        
        if (c == '\n') {
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                print("\b \b");
            }
        } else if (c >= 32 && c <= 126) {
            buf[pos++] = c;
            sys_write(STDOUT, &c, 1);
        }
    }
    
    buf[pos] = '\0';
    print("\n");
    return pos;
}

void printf(const char* fmt, ...) {
    char buffer[1024];
    uint32_t* args = (uint32_t*)((char*)&fmt + sizeof(fmt));
    int arg_index = 0;
    int pos = 0;
    
    while (*fmt && pos < 1023) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char* s = (const char*)args[arg_index++];
                    while (*s && pos < 1023) {
                        buffer[pos++] = *s++;
                    }
                    break;
                }
                case 'd': {
                    int n = (int)args[arg_index++];
                    char tmp[12];
                    int i = 0;
                    if (n < 0) {
                        buffer[pos++] = '-';
                        n = -n;
                    }
                    if (n == 0) {
                        buffer[pos++] = '0';
                    } else {
                        while (n > 0 && i < 11) {
                            tmp[i++] = '0' + (n % 10);
                            n /= 10;
                        }
                        while (i > 0) {
                            buffer[pos++] = tmp[--i];
                        }
                    }
                    break;
                }
                case 'u': {
                    unsigned int n = (unsigned int)args[arg_index++];
                    char tmp[12];
                    int i = 0;
                    if (n == 0) {
                        buffer[pos++] = '0';
                    } else {
                        while (n > 0 && i < 11) {
                            tmp[i++] = '0' + (n % 10);
                            n /= 10;
                        }
                        while (i > 0) {
                            buffer[pos++] = tmp[--i];
                        }
                    }
                    break;
                }
                case 'x': {
                    unsigned int n = (unsigned int)args[arg_index++];
                    const char* hex = "0123456789abcdef";
                    char tmp[9];
                    int i = 0;
                    buffer[pos++] = '0';
                    buffer[pos++] = 'x';
                    if (n == 0) {
                        buffer[pos++] = '0';
                    } else {
                        while (n > 0 && i < 8) {
                            tmp[i++] = hex[n & 0xF];
                            n >>= 4;
                        }
                        while (i > 0) {
                            buffer[pos++] = tmp[--i];
                        }
                    }
                    break;
                }
                case 'c':
                    buffer[pos++] = (char)args[arg_index++];
                    break;
                case '%':
                    buffer[pos++] = '%';
                    break;
            }
        } else {
            buffer[pos++] = *fmt;
        }
        fmt++;
    }
    
    buffer[pos] = '\0';
    print(buffer);
}

/* String utilities */
size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = s1;
    const uint8_t* p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/* Conversion utilities */
int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

char* itoa(int value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);
    
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return str;
}