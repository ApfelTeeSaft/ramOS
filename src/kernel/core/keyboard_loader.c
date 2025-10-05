/* keyboard_loader.c - Dynamic keyboard layout loader */

#include "keyboard_loader.h"
#include "keyboard_layout.h"
#include "../fs/vfs.h"
#include "../mm/heap.h"
#include "console.h"

#define MAX_LAYOUTS 32
#define MAX_LINE 256
#define LAYOUT_DIR "/etc/keyboard/layouts"
#define CONFIG_FILE "/etc/keyboard/active.conf"

/* File open flags - must match vfs.c */
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0004
#define O_CREAT     0x0008
#define O_TRUNC     0x0010
#define O_APPEND    0x0020

/* Loaded layouts */
static keyboard_layout_runtime_t loaded_layouts[MAX_LAYOUTS];
static int layout_count = 0;
static char current_layout_code[32] = "en_US";

/* Simple snprintf implementation - moved to top */
static void my_snprintf(char* str, size_t size, const char* format, ...) {
    uint32_t* args = (uint32_t*)((char*)&format + sizeof(format));
    int pos = 0;
    int arg_idx = 0;
    
    while (*format && pos < (int)size - 1) {
        if (*format == '%' && *(format + 1) == 's') {
            const char* s = (const char*)args[arg_idx++];
            while (*s && pos < (int)size - 1) {
                str[pos++] = *s++;
            }
            format += 2;
        } else {
            str[pos++] = *format++;
        }
    }
    
    str[pos] = '\0';
}

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

static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

/* Parse hex value */
static uint8_t parse_hex(const char* str) {
    uint8_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str) {
        val <<= 4;
        if (*str >= '0' && *str <= '9') {
            val += *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            val += *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            val += *str - 'A' + 10;
        } else {
            break;
        }
        str++;
    }
    return val;
}

/* Parse escape sequences */
static char parse_escape(const char* str, int* len) {
    if (str[0] != '\\') {
        *len = 1;
        return str[0];
    }
    
    *len = 2;
    switch (str[1]) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case 'b':  return '\b';
        case '0':  return '\0';
        case '\\': return '\\';
        case 'x':  /* Hex escape */
            if (str[2] && str[3]) {
                *len = 4;
                return (char)parse_hex(str);
            }
            return 0;
        default:
            return str[1];
    }
}

/* Trim whitespace */
static void trim(char* str) {
    /* Trim leading */
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) {
        char* p = str;
        while (*start) *p++ = *start++;
        *p = '\0';
    }
    
    /* Trim trailing */
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || 
                       str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = '\0';
    }
}

/* Parse layout file */
static int parse_layout_file(const char* filename, keyboard_layout_runtime_t* layout) {
    int fd = vfs_open(filename, 0);
    if (fd < 0) {
        return -1;
    }
    
    /* Initialize layout */
    memset(layout, 0, sizeof(keyboard_layout_runtime_t));
    
    char buffer[4096];
    int bytes = vfs_read(fd, buffer, sizeof(buffer) - 1);
    vfs_close(fd);
    
    if (bytes <= 0) return -1;
    buffer[bytes] = '\0';
    
    /* Parse line by line */
    char* line = buffer;
    int in_metadata = 0;
    int in_layout = 0;
    
    while (*line) {
        char current_line[MAX_LINE];
        int line_len = 0;
        
        /* Extract line */
        while (*line && *line != '\n' && line_len < MAX_LINE - 1) {
            current_line[line_len++] = *line++;
        }
        current_line[line_len] = '\0';
        if (*line == '\n') line++;
        
        trim(current_line);
        
        /* Skip empty lines and comments */
        if (current_line[0] == '\0' || current_line[0] == '#') {
            continue;
        }
        
        /* Check for sections */
        if (strcmp(current_line, "[metadata]") == 0) {
            in_metadata = 1;
            in_layout = 0;
            continue;
        } else if (strcmp(current_line, "[layout]") == 0) {
            in_metadata = 0;
            in_layout = 1;
            continue;
        }
        
        /* Parse key=value */
        char* equals = current_line;
        while (*equals && *equals != '=') equals++;
        if (*equals != '=') continue;
        
        *equals = '\0';
        char* key = current_line;
        char* value = equals + 1;
        
        trim(key);
        trim(value);
        
        if (in_metadata) {
            if (strcmp(key, "name") == 0) {
                strncpy(layout->name, value, 63);
            } else if (strcmp(key, "code") == 0) {
                strncpy(layout->code, value, 31);
            } else if (strcmp(key, "variant") == 0) {
                strncpy(layout->variant, value, 31);
            } else if (strcmp(key, "description") == 0) {
                strncpy(layout->description, value, 127);
            }
        } else if (in_layout) {
            /* Parse scancode mapping: 0x1E=a|A */
            uint8_t scancode = parse_hex(key);
            
            /* Find pipe separator */
            char* pipe = value;
            while (*pipe && *pipe != '|') pipe++;
            if (*pipe != '|') continue;
            
            *pipe = '\0';
            char* normal = value;
            char* shifted = pipe + 1;
            
            /* Parse normal char */
            int len;
            layout->normal[scancode] = parse_escape(normal, &len);
            
            /* Parse shifted char */
            layout->shifted[scancode] = parse_escape(shifted, &len);
        }
    }
    
    return 0;
}

/* Load all layouts from directory */
int keyboard_load_layouts(void) {
    kprintf("[KEYBOARD] Loading keyboard layouts from %s\n", LAYOUT_DIR);
    
    layout_count = 0;
    
    /* Open layouts directory */
    int dir_fd = vfs_open(LAYOUT_DIR, 0);
    if (dir_fd < 0) {
        kprintf("[KEYBOARD] Warning: Layouts directory not found\n");
        return -1;
    }
    
    /* Read directory entries */
    dirent_t entry;
    while (vfs_readdir(dir_fd, &entry) > 0) {
        /* Check for .conf extension */
        int name_len = strlen(entry.name);
        if (name_len < 6 || strcmp(entry.name + name_len - 5, ".conf") != 0) {
            continue;
        }
        
        if (layout_count >= MAX_LAYOUTS) {
            kprintf("[KEYBOARD] Warning: Too many layouts\n");
            break;
        }
        
        /* Build full path */
        char path[256];
        my_snprintf(path, sizeof(path), "%s/%s", LAYOUT_DIR, entry.name);
        
        /* Parse layout file */
        if (parse_layout_file(path, &loaded_layouts[layout_count]) == 0) {
            kprintf("[KEYBOARD] Loaded layout: %s (%s)\n", 
                   loaded_layouts[layout_count].name,
                   loaded_layouts[layout_count].code);
            layout_count++;
        } else {
            kprintf("[KEYBOARD] Failed to parse: %s\n", entry.name);
        }
    }
    
    vfs_close(dir_fd);
    
    kprintf("[KEYBOARD] Loaded %d keyboard layout(s)\n", layout_count);
    return layout_count;
}

/* Get layout by code */
keyboard_layout_runtime_t* keyboard_get_layout_by_code(const char* code) {
    for (int i = 0; i < layout_count; i++) {
        if (strcmp(loaded_layouts[i].code, code) == 0) {
            return &loaded_layouts[i];
        }
    }
    return NULL;
}

/* List all available layouts */
void keyboard_list_layouts(void) {
    kprintf("\nAvailable keyboard layouts:\n");
    kprintf("%-10s %-30s %-10s\n", "CODE", "NAME", "VARIANT");
    kprintf("%-10s %-30s %-10s\n", "----", "----", "-------");
    
    for (int i = 0; i < layout_count; i++) {
        char marker = (strcmp(loaded_layouts[i].code, current_layout_code) == 0) ? '*' : ' ';
        kprintf("%c %-8s %-30s %-10s\n", marker,
               loaded_layouts[i].code,
               loaded_layouts[i].name,
               loaded_layouts[i].variant);
    }
    
    kprintf("\n* = active layout\n");
}

/* Set active layout */
int keyboard_set_active_layout(const char* code) {
    keyboard_layout_runtime_t* layout = keyboard_get_layout_by_code(code);
    if (!layout) {
        kprintf("[KEYBOARD] Layout not found: %s\n", code);
        return -1;
    }
    
    /* Update keyboard driver */
    extern void keyboard_set_layout_runtime(keyboard_layout_runtime_t* layout);
    keyboard_set_layout_runtime(layout);
    
    /* Save to config */
    strncpy(current_layout_code, code, sizeof(current_layout_code) - 1);
    keyboard_save_config();
    
    kprintf("[KEYBOARD] Active layout: %s (%s)\n", layout->name, layout->code);
    return 0;
}

/* Get current layout code */
const char* keyboard_get_current_layout(void) {
    return current_layout_code;
}

/* Load config from file */
int keyboard_load_config(void) {
    int fd = vfs_open(CONFIG_FILE, 0);
    if (fd < 0) {
        /* No config file, use default */
        strcpy(current_layout_code, "en_US");
        return 0;
    }
    
    char buffer[256];
    int bytes = vfs_read(fd, buffer, sizeof(buffer) - 1);
    vfs_close(fd);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        
        /* Parse layout= line */
        char* line = buffer;
        while (*line) {
            if (strncmp(line, "layout=", 7) == 0) {
                line += 7;
                char* end = line;
                while (*end && *end != '\n' && *end != '\r') end++;
                *end = '\0';
                
                trim(line);
                strncpy(current_layout_code, line, sizeof(current_layout_code) - 1);
                break;
            }
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
        }
    }
    
    return 0;
}

/* Save config to file */
int keyboard_save_config(void) {
    int fd = vfs_open(CONFIG_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        kprintf("[KEYBOARD] Warning: Could not save config\n");
        return -1;
    }
    
    char buffer[256];
    my_snprintf(buffer, sizeof(buffer), "# ramOS Keyboard Configuration\nlayout=%s\n", 
             current_layout_code);
    
    vfs_write(fd, buffer, strlen(buffer));
    vfs_close(fd);
    
    return 0;
}

/* Initialize keyboard layouts */
void keyboard_layouts_init(void) {
    kprintf("[KEYBOARD] Initializing keyboard layout system...\n");
    
    /* Load all available layouts */
    keyboard_load_layouts();
    
    /* Load saved configuration */
    keyboard_load_config();
    
    /* Set active layout */
    keyboard_layout_runtime_t* layout = keyboard_get_layout_by_code(current_layout_code);
    if (layout) {
        extern void keyboard_set_layout_runtime(keyboard_layout_runtime_t* layout);
        keyboard_set_layout_runtime(layout);
        kprintf("[KEYBOARD] Active layout: %s\n", layout->name);
    } else {
        /* Fallback to first available layout */
        if (layout_count > 0) {
            keyboard_set_active_layout(loaded_layouts[0].code);
        }
    }
}