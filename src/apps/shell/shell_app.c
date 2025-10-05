/* shell_app.c - Standalone shell application */

#include "../../api/libsys.h"

#define MAX_INPUT 256
#define MAX_ARGS 16

/* Simple snprintf - MUST be defined before use */
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

/* Execute external program */
static int execute_program(char* argv[]) {
    int pid = sys_fork();
    
    if (pid < 0) {
        println("Error: Failed to fork");
        return -1;
    } else if (pid == 0) {
        /* Child process */
        if (sys_exec(argv[0], argv) < 0) {
            printf("Error: Failed to execute: %s\n", argv[0]);
            sys_exit(1);
        }
        /* Should never reach here */
        sys_exit(0);
    } else {
        /* Parent process - wait for child */
        int status;
        sys_wait(&status);
        return status;
    }
    
    return 0;
}

/* Built-in: cd */
static void builtin_cd(char* argv[]) {
    if (!argv[1]) {
        sys_chdir("/");
    } else {
        if (sys_chdir(argv[1]) < 0) {
            printf("cd: %s: No such directory\n", argv[1]);
        }
    }
}

/* Built-in: pwd */
static void builtin_pwd(void) {
    char cwd[256];
    if (sys_getcwd(cwd, sizeof(cwd)) >= 0) {
        println(cwd);
    }
}

/* Built-in: help */
static void builtin_help(void) {
    println("\nramOS Shell - Built-in Commands:");
    println("  cd [dir]     - Change directory");
    println("  pwd          - Print working directory");
    println("  help         - Show this help");
    println("  exit         - Exit shell");
    println("\nAvailable Applications:");
    println("  /bin/calculator   - Calculator");
    println("  /bin/editor       - Text Editor");
    println("  /bin/filemanager  - File Manager");
    println("");
}

/* Parse command line into arguments */
static int parse_args(char* input, char* argv[]) {
    int argc = 0;
    int in_word = 0;
    
    for (char* p = input; *p && argc < MAX_ARGS - 1; p++) {
        if (*p == ' ' || *p == '\t') {
            if (in_word) {
                *p = '\0';
                in_word = 0;
            }
        } else {
            if (!in_word) {
                argv[argc++] = p;
                in_word = 1;
            }
        }
    }
    
    argv[argc] = NULL;
    return argc;
}

/* Main shell loop */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[MAX_INPUT];
    char* cmd_argv[MAX_ARGS];
    char cwd[256];
    
    println("========================================");
    println("  ramOS Shell v1.0");
    println("========================================");
    println("\nType 'help' for available commands\n");
    
    while (1) {
        /* Show prompt with current directory */
        if (sys_getcwd(cwd, sizeof(cwd)) < 0) {
            strcpy(cwd, "/");
        }
        printf("%s $ ", cwd);
        
        /* Read command */
        readln(input, MAX_INPUT);
        
        /* Parse arguments */
        int cmd_argc = parse_args(input, cmd_argv);
        
        if (cmd_argc == 0) {
            continue;
        }
        
        /* Check for built-in commands */
        if (strcmp(cmd_argv[0], "exit") == 0) {
            break;
        } else if (strcmp(cmd_argv[0], "cd") == 0) {
            builtin_cd(cmd_argv);
        } else if (strcmp(cmd_argv[0], "pwd") == 0) {
            builtin_pwd();
        } else if (strcmp(cmd_argv[0], "help") == 0) {
            builtin_help();
        } else {
            /* Try to execute as external command */
            /* First check if path is absolute */
            if (cmd_argv[0][0] == '/') {
                execute_program(cmd_argv);
            } else {
                /* Try /bin/ directory */
                char path[MAX_INPUT];
                snprintf(path, sizeof(path), "/bin/%s", cmd_argv[0]);
                
                stat_t st;
                if (sys_stat(path, &st) == 0) {
                    /* File exists in /bin, execute it */
                    cmd_argv[0] = path;
                    execute_program(cmd_argv);
                } else {
                    printf("%s: command not found\n", cmd_argv[0]);
                }
            }
        }
    }
    
    println("Shell exited");
    return 0;
}