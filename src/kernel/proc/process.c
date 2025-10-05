/* process.c - Process management implementation (stub) */

#include "process.h"
#include "elf.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../core/timer.h"
#include "../core/console.h"

#define MAX_PROCESSES 64

static process_t* process_list = NULL;
static process_t* current_process = NULL;
static uint32_t next_pid = 1;

/* String utilities */
static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

/* Initialize process management */
void process_init(void) {
    kprintf("[PROC] Initializing process management...\n");
    process_list = NULL;
    current_process = NULL;
    next_pid = 1;
}

/* Create new process */
process_t* process_create(const char* name) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) return NULL;
    
    proc->pid = next_pid++;
    proc->parent_pid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_READY;
    proc->page_directory = vmm_create_page_directory();
    proc->esp = 0;
    proc->ebp = 0;
    proc->eip = 0;
    strncpy(proc->name, name, 64);
    strncpy(proc->cwd, "/", 256);
    proc->exit_code = 0;
    proc->start_time = timer_get_ticks();
    proc->fd_table = NULL;
    proc->fd_count = 0;
    proc->next = process_list;
    
    process_list = proc;
    
    return proc;
}

/* Fork current process */
process_t* process_fork(process_t* parent) {
    if (!parent) return NULL;
    
    /* Stub implementation */
    kprintf("[PROC] Fork not yet implemented\n");
    return NULL;
}

/* Execute program */
int process_exec(process_t* proc, const char* path, char* const argv[]) {
    if (!proc || !path) return -1;
    
    (void)argv;
    
    /* Load ELF binary */
    uint32_t entry = elf_load(path);
    if (entry == 0) {
        kprintf("[PROC] Failed to load: %s\n", path);
        return -1;
    }
    
    proc->eip = entry;
    proc->state = PROCESS_READY;
    
    return 0;
}

/* Exit process */
void process_exit(process_t* proc) {
    if (!proc) return;
    
    proc->state = PROCESS_DEAD;
    
    /* Remove from process list */
    process_t** current = &process_list;
    while (*current) {
        if (*current == proc) {
            *current = proc->next;
            break;
        }
        current = &(*current)->next;
    }
    
    /* Free resources */
    kfree(proc);
}

/* Wait for child process */
int process_wait(process_t* proc, int* status) {
    (void)proc;
    (void)status;
    
    /* Stub implementation */
    return -1;
}

/* Get current process */
process_t* process_get_current(void) {
    return current_process;
}

/* Switch to next process */
void process_switch(process_t* next) {
    if (!next) return;
    
    current_process = next;
    next->state = PROCESS_RUNNING;
    
    /* Switch page directory if different */
    if (next->page_directory) {
        vmm_switch_page_directory(next->page_directory);
    }
    
    /* Switch context (stub - would save/restore registers) */
}

/* Get process by PID */
process_t* process_get_by_pid(uint32_t pid) {
    for (process_t* proc = process_list; proc != NULL; proc = proc->next) {
        if (proc->pid == pid) {
            return proc;
        }
    }
    return NULL;
}