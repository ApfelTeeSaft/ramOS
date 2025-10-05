/* filemanager.c - Simple file manager application */

#include "../../api/libsys.h"

#define MAX_PATH 256
#define MAX_INPUT 256

static char current_path[MAX_PATH] = "/";

/* Display current directory contents */
static void list_directory(void) {
    int fd = sys_open(current_path, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot open directory: %s\n", current_path);
        return;
    }
    
    println("\n--- Directory Listing ---");
    printf("Path: %s\n\n", current_path);
    
    dirent_t entry;
    int count = 0;
    
    while (sys_readdir(fd, &entry) > 0) {
        const char* type_str = (entry.type == S_IFDIR) ? "[DIR] " : "[FILE]";
        printf("  %s %s\n", type_str, entry.name);
        count++;
    }
    
    printf("\n--- %d items ---\n\n", count);
    sys_close(fd);
}

/* Display file contents */
static void view_file(const char* filename) {
    char path[MAX_PATH];
    
    /* Build full path */
    if (filename[0] == '/') {
        strncpy(path, filename, MAX_PATH);
    } else {
        snprintf(path, MAX_PATH, "%s/%s", current_path, filename);
    }
    
    int fd = sys_open(path, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot open file: %s\n", filename);
        return;
    }
    
    stat_t st;
    if (sys_stat(path, &st) < 0) {
        printf("Error: Cannot stat file: %s\n", filename);
        sys_close(fd);
        return;
    }
    
    printf("\n--- File: %s (%u bytes) ---\n", filename, st.st_size);
    
    char buffer[4096];
    int bytes;
    
    while ((bytes = sys_read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';
        print(buffer);
    }
    
    println("\n--- End of file ---\n");
    sys_close(fd);
}

/* Change directory */
static void change_directory(const char* path) {
    char new_path[MAX_PATH];
    
    if (path[0] == '/') {
        strncpy(new_path, path, MAX_PATH);
    } else if (strcmp(path, "..") == 0) {
        /* Go up one directory */
        if (strcmp(current_path, "/") != 0) {
            strncpy(new_path, current_path, MAX_PATH);
            char* last_slash = new_path;
            for (char* p = new_path; *p; p++) {
                if (*p == '/') last_slash = p;
            }
            if (last_slash != new_path) {
                *last_slash = '\0';
            } else {
                strcpy(new_path, "/");
            }
        } else {
            strcpy(new_path, "/");
        }
    } else {
        snprintf(new_path, MAX_PATH, "%s/%s", current_path, path);
    }
    
    /* Verify directory exists */
    stat_t st;
    if (sys_stat(new_path, &st) < 0 || !(st.st_mode & S_IFDIR)) {
        printf("Error: Directory not found: %s\n", path);
        return;
    }
    
    strncpy(current_path, new_path, MAX_PATH);
    printf("Changed to: %s\n", current_path);
}

/* Create directory */
static void make_directory(const char* name) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", current_path, name);
    
    if (sys_mkdir(path, 0755) < 0) {
        printf("Error: Cannot create directory: %s\n", name);
    } else {
        printf("Created directory: %s\n", name);
    }
}

/* Delete file */
static void delete_file(const char* name) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", current_path, name);
    
    if (sys_unlink(path) < 0) {
        printf("Error: Cannot delete file: %s\n", name);
    } else {
        printf("Deleted: %s\n", name);
    }
}

/* Show help */
static void show_help(void) {
    println("\nFile Manager Commands:");
    println("  ls           - List directory contents");
    println("  cd <dir>     - Change directory");
    println("  cat <file>   - View file contents");
    println("  mkdir <dir>  - Create directory");
    println("  rm <file>    - Delete file");
    println("  pwd          - Print working directory");
    println("  stat <file>  - Show file information");
    println("  help         - Show this help");
    println("  quit         - Exit file manager");
    println("");
}

/* Show file statistics */
static void show_stat(const char* name) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", current_path, name);
    
    stat_t st;
    if (sys_stat(path, &st) < 0) {
        printf("Error: Cannot stat: %s\n", name);
        return;
    }
    
    printf("\nFile Information: %s\n", name);
    printf("  Type:   %s\n", (st.st_mode & S_IFDIR) ? "Directory" : "Regular File");
    printf("  Size:   %u bytes\n", st.st_size);
    printf("  Blocks: %u\n", st.st_blocks);
    printf("  Access: %u\n", st.st_atime);
    printf("  Modify: %u\n", st.st_mtime);
    printf("  Change: %u\n", st.st_ctime);
    println("");
}

/* Simple snprintf */
static void snprintf(char* str, size_t size, const char* format, ...) {
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

/* Main */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[MAX_INPUT];
    char command[64];
    char arg[MAX_PATH];
    
    println("========================================");
    println("  ramOS File Manager v1.0");
    println("========================================");
    show_help();
    
    while (1) {
        printf("%s> ", current_path);
        readln(input, MAX_INPUT);
        
        /* Parse command and argument */
        int i = 0, j = 0;
        
        /* Skip leading spaces */
        while (input[i] == ' ') i++;
        
        /* Extract command */
        while (input[i] && input[i] != ' ' && j < 63) {
            command[j++] = input[i++];
        }
        command[j] = '\0';
        
        /* Skip spaces */
        while (input[i] == ' ') i++;
        
        /* Extract argument */
        j = 0;
        while (input[i] && j < MAX_PATH - 1) {
            arg[j++] = input[i++];
        }
        arg[j] = '\0';
        
        /* Process commands */
        if (command[0] == '\0') {
            continue;
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "ls") == 0) {
            list_directory();
        } else if (strcmp(command, "cd") == 0) {
            if (arg[0]) {
                change_directory(arg);
            } else {
                println("Usage: cd <directory>");
            }
        } else if (strcmp(command, "cat") == 0) {
            if (arg[0]) {
                view_file(arg);
            } else {
                println("Usage: cat <filename>");
            }
        } else if (strcmp(command, "mkdir") == 0) {
            if (arg[0]) {
                make_directory(arg);
            } else {
                println("Usage: mkdir <directory>");
            }
        } else if (strcmp(command, "rm") == 0) {
            if (arg[0]) {
                delete_file(arg);
            } else {
                println("Usage: rm <filename>");
            }
        } else if (strcmp(command, "pwd") == 0) {
            println(current_path);
        } else if (strcmp(command, "stat") == 0) {
            if (arg[0]) {
                show_stat(arg);
            } else {
                println("Usage: stat <filename>");
            }
        } else if (strcmp(command, "help") == 0) {
            show_help();
        } else {
            printf("Unknown command: %s\n", command);
            println("Type 'help' for available commands");
        }
    }
    
    println("File Manager closed");
    return 0;
}