/* kbmap.c - Keyboard layout management utility */

#include "../../api/libsys.h"

#define MAX_LAYOUTS 32

typedef struct {
    char name[64];
    char code[32];
    char variant[32];
    char description[128];
} layout_info_t;

static layout_info_t layouts[MAX_LAYOUTS];
static int layout_count = 0;

/* Load available layouts from directory */
static int load_layouts(void) {
    int fd = sys_open("/etc/keyboard/layouts", O_RDONLY);
    if (fd < 0) {
        println("Error: Could not open layouts directory");
        return -1;
    }
    
    dirent_t entry;
    layout_count = 0;
    
    while (sys_readdir(fd, &entry) > 0 && layout_count < MAX_LAYOUTS) {
        /* Check for .layout extension */
        int name_len = strlen(entry.name);
        if (name_len < 7 || strcmp(entry.name + name_len - 7, ".layout") != 0) {
            continue;
        }
        
        /* Build full path */
        char path[256];
        printf("%s/%s", "/etc/keyboard/layouts", entry.name);
        int path_len = strlen("/etc/keyboard/layouts/");
        strcpy(path, "/etc/keyboard/layouts/");
        strcpy(path + path_len, entry.name);
        
        /* Open and parse layout file */
        int layout_fd = sys_open(path, O_RDONLY);
        if (layout_fd < 0) continue;
        
        char buffer[2048];
        int bytes = sys_read(layout_fd, buffer, sizeof(buffer) - 1);
        sys_close(layout_fd);
        
        if (bytes <= 0) continue;
        buffer[bytes] = '\0';
        
        /* Parse metadata */
        layout_info_t* layout = &layouts[layout_count];
        memset(layout, 0, sizeof(layout_info_t));
        
        char* line = buffer;
        int in_metadata = 0;
        
        while (*line) {
            /* Extract line */
            char current_line[256];
            int i = 0;
            while (*line && *line != '\n' && i < 255) {
                current_line[i++] = *line++;
            }
            current_line[i] = '\0';
            if (*line == '\n') line++;
            
            /* Trim */
            char* start = current_line;
            while (*start == ' ' || *start == '\t') start++;
            
            if (strcmp(start, "[metadata]") == 0) {
                in_metadata = 1;
                continue;
            } else if (start[0] == '[') {
                in_metadata = 0;
                continue;
            }
            
            if (!in_metadata || start[0] == '#' || start[0] == '\0') {
                continue;
            }
            
            /* Parse key=value */
            char* equals = start;
            while (*equals && *equals != '=') equals++;
            if (*equals != '=') continue;
            
            *equals = '\0';
            char* key = start;
            char* value = equals + 1;
            
            /* Trim value */
            while (*value == ' ' || *value == '\t') value++;
            int len = strlen(value);
            while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t' || 
                              value[len-1] == '\r' || value[len-1] == '\n')) {
                value[--len] = '\0';
            }
            
            if (strcmp(key, "name") == 0) {
                strncpy(layout->name, value, 63);
            } else if (strcmp(key, "code") == 0) {
                strncpy(layout->code, value, 31);
            } else if (strcmp(key, "variant") == 0) {
                strncpy(layout->variant, value, 31);
            } else if (strcmp(key, "description") == 0) {
                strncpy(layout->description, value, 127);
            }
        }
        
        if (layout->code[0] != '\0') {
            layout_count++;
        }
    }
    
    sys_close(fd);
    return layout_count;
}

/* Get current active layout */
static int get_current_layout(char* code_buf, size_t size) {
    int fd = sys_open("/etc/keyboard/active.conf", O_RDONLY);
    if (fd < 0) {
        strcpy(code_buf, "en_US");
        return 0;
    }
    
    char buffer[256];
    int bytes = sys_read(fd, buffer, sizeof(buffer) - 1);
    sys_close(fd);
    
    if (bytes <= 0) {
        strcpy(code_buf, "en_US");
        return 0;
    }
    
    buffer[bytes] = '\0';
    
    /* Find layout= line */
    char* line = buffer;
    while (*line) {
        if (strncmp(line, "layout=", 7) == 0) {
            line += 7;
            char* end = line;
            while (*end && *end != '\n' && *end != '\r') end++;
            *end = '\0';
            
            /* Trim */
            while (*line == ' ' || *line == '\t') line++;
            
            strncpy(code_buf, line, size - 1);
            code_buf[size - 1] = '\0';
            return 0;
        }
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    
    strcpy(code_buf, "en_US");
    return 0;
}

/* Set active layout */
static int set_layout(const char* code) {
    /* Verify layout exists */
    int found = 0;
    for (int i = 0; i < layout_count; i++) {
        if (strcmp(layouts[i].code, code) == 0) {
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf("Error: Layout '%s' not found\n", code);
        return -1;
    }
    
    /* Write config file */
    int fd = sys_open("/etc/keyboard/active.conf", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        println("Error: Could not write configuration file");
        println("Note: Changes will not persist across reboots in RAM disk mode");
        return -1;
    }
    
    char buffer[256];
    printf("%s%s%s", "# ramOS Keyboard Configuration\nlayout=", code, "\n");
    
    strcpy(buffer, "# ramOS Keyboard Configuration\nlayout=");
    strcat(buffer, code);
    strcat(buffer, "\n");
    
    sys_write(fd, buffer, strlen(buffer));
    sys_close(fd);
    
    printf("Layout changed to: %s\n", code);
    println("Note: Restart the system for changes to take effect");
    
    return 0;
}

/* List all layouts */
static void list_layouts(void) {
    if (layout_count == 0) {
        println("No keyboard layouts found");
        return;
    }
    
    char current[32];
    get_current_layout(current, sizeof(current));
    
    println("\nAvailable keyboard layouts:");
    println("================================================================================");
    printf("%-3s %-10s %-30s %-12s\n", "", "CODE", "NAME", "VARIANT");
    println("--------------------------------------------------------------------------------");
    
    for (int i = 0; i < layout_count; i++) {
        char marker = (strcmp(layouts[i].code, current) == 0) ? '*' : ' ';
        printf("%c  %-10s %-30s %-12s\n", 
               marker,
               layouts[i].code,
               layouts[i].name,
               layouts[i].variant);
    }
    
    println("================================================================================");
    println("* = currently active layout");
    println("");
}

/* Show help */
static void show_help(void) {
    println("kbmap - Keyboard Layout Management Utility");
    println("");
    println("Usage:");
    println("  kbmap                  List all available layouts");
    println("  kbmap list             List all available layouts");
    println("  kbmap set <code>       Set active layout (e.g., en_US, de_DE)");
    println("  kbmap current          Show current active layout");
    println("  kbmap help             Show this help");
    println("");
    println("Examples:");
    println("  kbmap                  # List layouts");
    println("  kbmap set de_DE        # Switch to German layout");
    println("  kbmap set en_US        # Switch to US layout");
    println("");
}

int main(int argc, char* argv[]) {
    /* Load available layouts */
    if (load_layouts() < 0) {
        println("Error: Could not load keyboard layouts");
        return 1;
    }
    
    /* Parse command */
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "list") == 0)) {
        /* List layouts */
        list_layouts();
    } else if (argc == 2 && strcmp(argv[1], "current") == 0) {
        /* Show current layout */
        char current[32];
        get_current_layout(current, sizeof(current));
        
        /* Find layout info */
        for (int i = 0; i < layout_count; i++) {
            if (strcmp(layouts[i].code, current) == 0) {
                printf("Current layout: %s (%s)\n", layouts[i].name, layouts[i].code);
                printf("Variant: %s\n", layouts[i].variant);
                if (layouts[i].description[0]) {
                    printf("Description: %s\n", layouts[i].description);
                }
                return 0;
            }
        }
        
        printf("Current layout: %s\n", current);
    } else if (argc == 3 && strcmp(argv[1], "set") == 0) {
        /* Set layout */
        return set_layout(argv[2]);
    } else if (argc == 2 && strcmp(argv[1], "help") == 0) {
        show_help();
    } else {
        println("Error: Invalid command");
        println("Run 'kbmap help' for usage information");
        return 1;
    }
    
    return 0;
}