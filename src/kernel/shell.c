/* shell.c - Interactive command shell (updated for VFS) */

#include "shell.h"
#include "core/console.h"
#include "drivers/keyboard.h"
#include "mm/memory.h"
#include "core/timer.h"
#include "fs/initrd.h"
#include "fs/vfs.h"

/* CPU vendor string retrieval */
static void get_cpu_vendor(char* vendor) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0));
    
    *(uint32_t*)(vendor + 0) = ebx;
    *(uint32_t*)(vendor + 4) = edx;
    *(uint32_t*)(vendor + 8) = ecx;
    vendor[12] = '\0';
}

/* String functions */
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

/* Command: help */
static void cmd_help(void) {
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Available commands:\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("  help     - Show this help message\n");
    kprintf("  mem      - Display memory statistics\n");
    kprintf("  uptime   - Show system uptime\n");
    kprintf("  echo     - Print text to console\n");
    kprintf("  ls       - List files in initrd\n");
    kprintf("  cat      - Display file contents\n");
    kprintf("  sysinfo  - Show system information\n");
    kprintf("  clear    - Clear screen\n");
    kprintf("\nApplications (run with full path):\n");
    kprintf("  /bin/calculator   - Calculator\n");
    kprintf("  /bin/editor       - Text Editor\n");
    kprintf("  /bin/filemanager  - File Manager\n");
    kprintf("  /bin/shell        - Standalone Shell\n");
    kprintf("  /bin/installer    - System Installer\n");
}

/* Command: mem */
static void cmd_mem(void) {
    uint32_t total = memory_get_total();
    uint32_t used = memory_get_used();
    uint32_t free = memory_get_free();
    
    kprintf("Memory Statistics:\n");
    kprintf("  Total: %u KB\n", total / 1024);
    kprintf("  Used:  %u KB\n", used / 1024);
    kprintf("  Free:  %u KB\n", free / 1024);
}

/* Command: uptime */
static void cmd_uptime(void) {
    uint32_t ms = timer_get_uptime_ms();
    uint32_t seconds = ms / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    kprintf("Uptime: %u hours, %u minutes, %u seconds\n", hours, minutes, seconds);
    kprintf("Ticks: %u (ms: %u)\n", timer_get_ticks(), ms);
}

/* Command: echo */
static void cmd_echo(const char* args) {
    if (*args) {
        kprintf("%s\n", args);
    }
}

/* Command: ls */
static void cmd_ls(void) {
    initrd_file_t* files;
    int count = initrd_list(&files);
    
    if (count == 0) {
        kprintf("No files in initrd\n");
        return;
    }
    
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Files in initrd (%d total):\n", count);
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    for (int i = 0; i < count; i++) {
        kprintf("  %-30s %u bytes\n", files[i].name, files[i].size);
    }
}

/* Command: cat */
static void cmd_cat(const char* args) {
    if (!*args) {
        kprintf("Usage: cat <filename>\n");
        return;
    }
    
    initrd_file_t* file = initrd_find(args);
    if (!file) {
        kprintf("Error: File '%s' not found\n", args);
        return;
    }
    
    /* Print file contents */
    for (uint32_t i = 0; i < file->size; i++) {
        console_putchar(file->data[i]);
    }
    
    /* Ensure newline at end */
    if (file->size > 0 && file->data[file->size - 1] != '\n') {
        console_putchar('\n');
    }
}

/* Command: sysinfo */
static void cmd_sysinfo(void) {
    char vendor[13];
    get_cpu_vendor(vendor);
    
    console_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("ramOS System Information\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("  Kernel Version: %s\n", KERNEL_VERSION);
    kprintf("  Architecture:   i686 (32-bit)\n");
    kprintf("  CPU Vendor:     %s\n", vendor);
    kprintf("  CPU Mode:       Protected Mode\n");
    kprintf("  Bootloader:     Multiboot v1 (GRUB2)\n");
    kprintf("  Features:       VFS, Drivers, Syscalls, Apps\n");
}

/* Command: clear */
static void cmd_clear(void) {
    console_clear();
}

/* Parse and execute command */
static void execute_command(char* input) {
    /* Skip leading spaces */
    while (*input == ' ') input++;
    
    /* Find command end */
    char* args = input;
    while (*args && *args != ' ') args++;
    
    /* Null-terminate command */
    if (*args) {
        *args = '\0';
        args++;
        /* Skip spaces before args */
        while (*args == ' ') args++;
    }
    
    /* Empty command */
    if (!*input) {
        return;
    }
    
    /* Execute commands */
    if (strcmp(input, "help") == 0) {
        cmd_help();
    } else if (strcmp(input, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(input, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(input, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(input, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(input, "cat") == 0) {
        cmd_cat(args);
    } else if (strcmp(input, "sysinfo") == 0) {
        cmd_sysinfo();
    } else if (strcmp(input, "clear") == 0) {
        cmd_clear();
    } else if (input[0] == '/') {
        /* Try to execute as application */
        kprintf("Application execution not yet implemented.\n");
        kprintf("This will be available once process management is complete.\n");
    } else {
        console_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Unknown command: %s\n", input);
        console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        kprintf("Type 'help' for a list of commands.\n");
    }
}

void shell_run(void) {
    char input[256];
    
    /* Print welcome banner */
    console_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("\n");
    kprintf("========================================\n");
    kprintf("  Welcome to ramOS v%s\n", KERNEL_VERSION);
    kprintf("========================================\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("\nType 'help' for available commands.\n\n");
    
    /* Main shell loop */
    while (1) {
        /* Print prompt */
        console_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("ramOS> ");
        console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        /* Read input */
        keyboard_read_line(input, sizeof(input));
        
        /* Execute command */
        execute_command(input);
    }
}