/* shell.c - Interactive command shell with process management */

#include "shell.h"
#include "core/console.h"
#include "core/keyboard.h"
#include "mm/memory.h"
#include "mm/heap.h"
#include "core/timer.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "proc/process.h"
#include "proc/elf.h"

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

static int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
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
    kprintf("\nProcess Management:\n");
    kprintf("  ps       - List running processes\n");
    kprintf("  kill     - Kill a process by PID\n");
    kprintf("  meminfo  - Show detailed memory info\n");
    kprintf("  exec     - Execute a program\n");
    kprintf("\nApplications (run with full path or use exec):\n");
    kprintf("  /bin/calculator   - Calculator\n");
    kprintf("  /bin/editor       - Text Editor\n");
    kprintf("  /bin/filemanager  - File Manager\n");
    kprintf("  /bin/shell        - Standalone Shell\n");
    kprintf("  /bin/installer    - System Installer\n");
    kprintf("  /bin/kbmap        - Keyboard Layout Manager\n");
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

/* Command: meminfo - detailed */
static void cmd_meminfo(void) {
    uint32_t total = memory_get_total();
    uint32_t used = memory_get_used();
    uint32_t free = memory_get_free();
    uint32_t heap_used = heap_get_used();
    uint32_t heap_free = heap_get_free();
    
    kprintf("Detailed Memory Information:\n");
    kprintf("  Physical Memory:\n");
    kprintf("    Total:     %u KB (%u MB)\n", total / 1024, total / (1024*1024));
    kprintf("    Used:      %u KB\n", used / 1024);
    kprintf("    Free:      %u KB\n", free / 1024);
    kprintf("  Heap:\n");
    kprintf("    Used:      %u KB\n", heap_used / 1024);
    kprintf("    Free:      %u KB\n", heap_free / 1024);
    kprintf("  Pages:\n");
    kprintf("    Page Size: 4 KB\n");
    kprintf("    Total:     %u pages\n", total / 4096);
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
    kprintf("  Features:       VFS, Drivers, Syscalls, Processes\n");
    kprintf("  Processes:      %d running\n", process_count());
}

/* Command: ps - list processes */
static void cmd_ps(void) {
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Running Processes:\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    process_list_all();
}

/* Command: kill - kill process */
static void cmd_kill(const char* args) {
    if (!*args) {
        kprintf("Usage: kill <pid>\n");
        return;
    }
    
    int pid = atoi(args);
    
    if (pid <= 0) {
        kprintf("Error: Invalid PID\n");
        return;
    }
    
    if (pid == 0) {
        kprintf("Error: Cannot kill kernel process\n");
        return;
    }
    
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        kprintf("Error: Process %d not found\n", pid);
        return;
    }
    
    kprintf("Killing process %d (%s)...\n", pid, proc->name);
    
    if (process_kill(pid, 9) == 0) {
        kprintf("Process %d killed successfully\n", pid);
    } else {
        kprintf("Failed to kill process %d\n", pid);
    }
}

/* Command: exec - execute program */
static void cmd_exec(const char* args) {
    if (!*args) {
        kprintf("Usage: exec <program> [args]\n");
        return;
    }
    
    /* Create process */
    process_t* proc = process_create(args);
    if (!proc) {
        kprintf("Error: Failed to create process\n");
        return;
    }
    
    /* Execute program */
    kprintf("Executing: %s (PID %d)\n", args, proc->pid);
    
    char* argv[] = { (char*)args, NULL };
    if (process_exec(proc, args, argv) < 0) {
        kprintf("Error: Failed to execute %s\n", args);
        process_kill(proc->pid, 9);
        return;
    }
    
    kprintf("Process %d started\n", proc->pid);
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
    } else if (strcmp(input, "meminfo") == 0) {
        cmd_meminfo();
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
    } else if (strcmp(input, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(input, "kill") == 0) {
        cmd_kill(args);
    } else if (strcmp(input, "exec") == 0) {
        cmd_exec(args);
    } else if (input[0] == '/') {
        /* Try to execute as application */
        cmd_exec(input);
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