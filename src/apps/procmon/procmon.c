/* procmon.c - Process and Memory Monitor with Kill Support */

#include "../../api/libsys.h"

#define REFRESH_INTERVAL 2000  /* 2 seconds */
#define MAX_PROCS 64

/* Display header */
static void display_header(void) {
    println("========================================");
    println("  ramOS Process Monitor");
    println("========================================");
    println("");
}

/* Display memory info */
static void display_memory(void) {
    /* Get time info to show uptime */
    time_t t;
    sys_gettime(&t);
    
    uint32_t seconds = t.seconds;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    
    println("System Information:");
    printf("  Uptime: %u:%02u:%02u\n", hours, minutes, seconds);
    printf("  Ticks:  %u\n", t.ticks);
    println("");
}

/* Display process list */
static void display_processes(void) {
    proc_info_t procs[MAX_PROCS];
    int count = sys_getprocs(procs, MAX_PROCS);
    
    if (count < 0) {
        println("Error: Failed to get process list");
        return;
    }
    
    println("Running Processes:");
    println("  PID  PPID  STATE     CPU TIME  NAME");
    println("  ---- ----  --------  --------  --------------------");
    
    for (int i = 0; i < count; i++) {
        const char* state_str;
        switch (procs[i].state) {
            case PROC_STATE_READY:   state_str = "READY   "; break;
            case PROC_STATE_RUNNING: state_str = "RUNNING "; break;
            case PROC_STATE_BLOCKED: state_str = "BLOCKED "; break;
            case PROC_STATE_ZOMBIE:  state_str = "ZOMBIE  "; break;
            case PROC_STATE_DEAD:    state_str = "DEAD    "; break;
            default:                 state_str = "UNKNOWN "; break;
        }
        
        printf("  %-4u %-4u  %s  %-8u  %s\n", 
               procs[i].pid, 
               procs[i].ppid,
               state_str,
               procs[i].cpu_time,
               procs[i].name);
    }
    
    printf("\nTotal: %d processes\n", count);
    println("");
}

/* Display instructions */
static void display_help(void) {
    println("Commands:");
    println("  r       - Refresh display");
    println("  k <pid> - Kill process by PID");
    println("  h       - Show this help");
    println("  q       - Quit");
    println("");
}

/* Parse integer from string */
static int parse_int(const char* str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

/* Kill process command */
static void cmd_kill(const char* args) {
    /* Skip whitespace */
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        println("Usage: k <pid>");
        return;
    }
    
    int pid = parse_int(args);
    
    if (pid <= 0) {
        println("Error: Invalid PID");
        return;
    }
    
    if (pid == 0) {
        println("Error: Cannot kill kernel process (PID 0)");
        return;
    }
    
    printf("Killing process %d...\n", pid);
    
    int result = sys_kill(pid, 9);  /* SIGKILL */
    
    if (result == 0) {
        printf("Process %d killed successfully\n", pid);
    } else {
        printf("Failed to kill process %d (not found or access denied)\n", pid);
    }
}

/* Main loop */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[256];
    
    display_header();
    display_help();
    
    while (1) {
        /* Display information */
        display_memory();
        display_processes();
        
        print("procmon> ");
        readln(input, sizeof(input));
        
        /* Skip empty input */
        if (input[0] == '\0') {
            continue;
        }
        
        /* Parse command */
        char cmd = input[0];
        char* args = input + 1;
        
        /* Skip whitespace in args */
        while (*args == ' ') args++;
        
        switch (cmd) {
            case 'q':
                println("Exiting process monitor...");
                return 0;
                
            case 'r':
                println("Refreshing...");
                break;
                
            case 'k':
                cmd_kill(args);
                break;
                
            case 'h':
                display_help();
                break;
                
            default:
                printf("Unknown command: %c\n", cmd);
                println("Type 'h' for help.");
                break;
        }
    }
    
    return 0;
}