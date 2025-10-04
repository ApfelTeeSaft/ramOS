/* initrd.c - Initial RAM disk (cpio newc format) parser */

#include "initrd.h"
#include "memory.h"

/* Maximum files */
#define MAX_FILES 64

/* File list */
static initrd_file_t files[MAX_FILES];
static int file_count = 0;

/* String functions */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

/* Parse hex string */
static uint32_t parse_hex(const char* str, int len) {
    uint32_t val = 0;
    for (int i = 0; i < len; i++) {
        val <<= 4;
        char c = str[i];
        if (c >= '0' && c <= '9') {
            val += c - '0';
        } else if (c >= 'A' && c <= 'F') {
            val += c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            val += c - 'a' + 10;
        }
    }
    return val;
}

int initrd_init(uint32_t addr, uint32_t size) {
    uint8_t* data = (uint8_t*)addr;
    uint8_t* end = data + size;
    
    file_count = 0;
    
    while (data < end && file_count < MAX_FILES) {
        /* Check for cpio newc magic */
        if (data + 110 > end) break;
        
        if (data[0] != '0' || data[1] != '7' || data[2] != '0' ||
            data[3] != '7' || data[4] != '0' || data[5] != '1') {
            break;
        }
        
        /* Parse header */
        uint32_t namesize = parse_hex((char*)(data + 94), 8);
        uint32_t filesize = parse_hex((char*)(data + 54), 8);
        
        /* Get filename */
        char* filename = (char*)(data + 110);
        
        /* Check for trailer */
        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }
        
        /* Skip . and empty names */
        if (filename[0] == '.' && (filename[1] == '\0' || filename[1] == '/')) {
            /* Align to 4 bytes */
            uint32_t header_size = ((110 + namesize + 3) & ~3);
            uint32_t data_size = ((filesize + 3) & ~3);
            data += header_size + data_size;
            continue;
        }
        
        /* Store file info */
        if (file_count < MAX_FILES) {
            initrd_file_t* f = &files[file_count];
            
            /* Copy filename (skip leading ./) */
            const char* name = filename;
            if (name[0] == '.' && name[1] == '/') {
                name += 2;
            }
            
            size_t name_len = strlen(name);
            if (name_len >= sizeof(f->name)) {
                name_len = sizeof(f->name) - 1;
            }
            memcpy(f->name, name, name_len);
            f->name[name_len] = '\0';
            
            f->size = filesize;
            
            /* Align to 4 bytes */
            uint32_t header_size = ((110 + namesize + 3) & ~3);
            f->data = data + header_size;
            
            file_count++;
            
            /* Move to next entry */
            uint32_t data_size = ((filesize + 3) & ~3);
            data += header_size + data_size;
        } else {
            break;
        }
    }
    
    return file_count;
}

int initrd_list(initrd_file_t** out_files) {
    *out_files = files;
    return file_count;
}

initrd_file_t* initrd_find(const char* name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return 0;
}