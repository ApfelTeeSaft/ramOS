/* process.h - Process management */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

/* Process states */
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE,
    PROCESS_DEAD
} process_state_t;

/* Process structure */
typedef struct process {
    uint32_t pid;                    /* Process ID */
    uint32_t parent_pid;             /* Parent process ID */
    process_state_t state;           /* Process state */
    
    uint32_t* page_directory;        /* Page directory */
    uint32_t esp;                    /* Stack pointer */
    uint32_t ebp;                    /* Base pointer */
    uint32_t eip;                    /* Instruction pointer */
    
    char name[64];                   /* Process name */
    char cwd[256];                   /* Current working directory */
    
    int exit_code;                   /* Exit code */
    uint32_t start_time;             /* Start time (ticks) */
    
    /* File descriptors */
    struct vfs_node** fd_table;      /* File descriptor table */
    uint32_t fd_count;               /* Number of open files */
    
    struct process* next;            /* Next process in list */
} process_t;

/* Initialize process management */
void process_init(void);

/* Create new process */
process_t* process_create(const char* name);

/* Fork current process */
process_t* process_fork(process_t* parent);

/* Execute program */
int process_exec(process_t* proc, const char* path, char* const argv[]);

/* Exit process */
void process_exit(process_t* proc);

/* Wait for child process */
int process_wait(process_t* proc, int* status);

/* Get current process */
process_t* process_get_current(void);

/* Switch to next process */
void process_switch(process_t* next);

/* Get process by PID */
process_t* process_get_by_pid(uint32_t pid);

#endif /* PROCESS_H */