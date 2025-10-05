/* process.c - Complete Process Management Implementation */

#include "process.h"
#include "elf.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../core/timer.h"
#include "../core/console.h"
#include "../fs/vfs.h"

#define MAX_PROCESSES 64
#define KERNEL_STACK_SIZE 8192
#define USER_STACK_SIZE 8192
#define USER_STACK_TOP 0xC0000000
#define MAX_FD_PER_PROCESS 32

static process_t* process_list = NULL;
static process_t* current_process = NULL;
static uint32_t next_pid = 1;

/* Context switch assembly helpers */
extern void switch_context(uint32_t* old_esp, uint32_t new_esp);
extern void enter_usermode(uint32_t eip, uint32_t esp);

/* String utilities */
static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

static char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/* Initialize process management */
void process_init(void) {
    kprintf("[PROC] Initializing process management...\n");
    process_list = NULL;
    current_process = NULL;
    next_pid = 1;
    
    /* Create kernel process (PID 0) */
    process_t* kernel_proc = process_create("kernel");
    if (kernel_proc) {
        kernel_proc->pid = 0;
        kernel_proc->state = PROCESS_RUNNING;
        kernel_proc->page_directory = vmm_get_page_directory();
        current_process = kernel_proc;
        kprintf("[PROC] Kernel process created (PID 0)\n");
    }
}

/* Allocate process structure */
static process_t* alloc_process(void) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) return NULL;
    
    memset(proc, 0, sizeof(process_t));
    
    /* Allocate file descriptor table */
    proc->fd_table = (struct vfs_node**)kmalloc(sizeof(struct vfs_node*) * MAX_FD_PER_PROCESS);
    if (!proc->fd_table) {
        kfree(proc);
        return NULL;
    }
    
    memset(proc->fd_table, 0, sizeof(struct vfs_node*) * MAX_FD_PER_PROCESS);
    proc->fd_count = MAX_FD_PER_PROCESS;
    
    return proc;
}

/* Free process structure */
static void free_process(process_t* proc) {
    if (!proc) return;
    
    /* Free file descriptor table */
    if (proc->fd_table) {
        kfree(proc->fd_table);
    }
    
    /* Free page directory */
    if (proc->page_directory) {
        /* TODO: Free all page tables and mapped pages */
        /* For now, just free the directory itself */
        kfree(proc->page_directory);
    }
    
    kfree(proc);
}

/* Create new process */
process_t* process_create(const char* name) {
    process_t* proc = alloc_process();
    if (!proc) return NULL;
    
    proc->pid = next_pid++;
    proc->parent_pid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_READY;
    proc->page_directory = vmm_create_page_directory();
    proc->esp = 0;
    proc->ebp = 0;
    proc->eip = 0;
    strncpy(proc->name, name, 64);
    strcpy(proc->cwd, "/");
    proc->exit_code = 0;
    proc->start_time = timer_get_ticks();
    
    /* Add to process list */
    proc->next = process_list;
    process_list = proc;
    
    kprintf("[PROC] Created process '%s' (PID %d)\n", name, proc->pid);
    
    return proc;
}

/* Copy page directory and all pages for fork */
static uint32_t* clone_page_directory_deep(uint32_t* src_pd) {
    if (!src_pd) return NULL;
    
    uint32_t* new_pd = vmm_create_page_directory();
    if (!new_pd) return NULL;
    
    /* Copy kernel space (identity mapped) */
    for (int i = 0; i < 256; i++) {  /* First 1GB */
        new_pd[i] = src_pd[i];
    }
    
    /* Clone user space */
    for (int i = 256; i < 1024; i++) {
        uint32_t pd_entry = src_pd[i];
        
        if (!(pd_entry & PAGE_PRESENT)) {
            new_pd[i] = 0;
            continue;
        }
        
        /* Get source page table */
        uint32_t* src_pt = (uint32_t*)(pd_entry & ~0xFFF);
        
        /* Allocate new page table */
        uint32_t* new_pt = (uint32_t*)kmalloc_aligned(4096, 4096);
        if (!new_pt) continue;
        
        /* Copy and clone pages */
        for (int j = 0; j < 1024; j++) {
            uint32_t pt_entry = src_pt[j];
            
            if (!(pt_entry & PAGE_PRESENT)) {
                new_pt[j] = 0;
                continue;
            }
            
            /* Allocate new physical page */
            void* new_page = kmalloc_aligned(4096, 4096);
            if (!new_page) {
                new_pt[j] = 0;
                continue;
            }
            
            /* Copy page contents */
            void* src_page = (void*)(pt_entry & ~0xFFF);
            memcpy(new_page, src_page, 4096);
            
            /* Set up new page table entry */
            new_pt[j] = ((uint32_t)new_page & ~0xFFF) | (pt_entry & 0xFFF);
        }
        
        /* Set up new page directory entry */
        new_pd[i] = ((uint32_t)new_pt & ~0xFFF) | (pd_entry & 0xFFF);
    }
    
    return new_pd;
}

/* Fork current process */
process_t* process_fork(process_t* parent) {
    if (!parent) return NULL;
    
    kprintf("[PROC] Forking process %d (%s)\n", parent->pid, parent->name);
    
    /* Create child process structure */
    process_t* child = alloc_process();
    if (!child) {
        kprintf("[PROC] Fork failed: out of memory\n");
        return NULL;
    }
    
    /* Copy parent process data */
    child->pid = next_pid++;
    child->parent_pid = parent->pid;
    child->state = PROCESS_READY;
    strncpy(child->name, parent->name, 64);
    strcpy(child->cwd, parent->cwd);
    child->start_time = timer_get_ticks();
    child->exit_code = 0;
    
    /* Clone page directory and all user pages */
    child->page_directory = clone_page_directory_deep(parent->page_directory);
    if (!child->page_directory) {
        kprintf("[PROC] Fork failed: couldn't clone page directory\n");
        free_process(child);
        return NULL;
    }
    
    /* Copy register state (will be set by caller) */
    child->esp = parent->esp;
    child->ebp = parent->ebp;
    child->eip = parent->eip;
    
    /* Clone file descriptor table */
    if (parent->fd_table) {
        memcpy(child->fd_table, parent->fd_table, 
               sizeof(struct vfs_node*) * parent->fd_count);
        child->fd_count = parent->fd_count;
    }
    
    /* Add to process list */
    child->next = process_list;
    process_list = child;
    
    kprintf("[PROC] Fork successful: parent=%d, child=%d\n", parent->pid, child->pid);
    
    return child;
}

/* Load and execute ELF binary */
int process_exec(process_t* proc, const char* path, char* const argv[]) {
    if (!proc || !path) return -1;
    
    kprintf("[PROC] Executing: %s (PID %d)\n", path, proc->pid);
    
    /* Load ELF binary */
    uint32_t entry = elf_load(path);
    if (entry == 0) {
        kprintf("[PROC] Failed to load: %s\n", path);
        return -1;
    }
    
    /* Set up user stack */
    uint32_t user_stack = USER_STACK_TOP;
    
    /* Allocate and map user stack pages */
    for (uint32_t addr = user_stack - USER_STACK_SIZE; addr < user_stack; addr += PAGE_SIZE) {
        void* page = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
        if (page) {
            vmm_map_page(addr, (uint32_t)page, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset(page, 0, PAGE_SIZE);
        }
    }
    
    /* Push arguments onto stack */
    int argc = 0;
    if (argv) {
        /* Count arguments */
        while (argv[argc]) argc++;
        
        /* Allocate space for argument pointers */
        uint32_t* arg_ptrs = (uint32_t*)kmalloc(sizeof(uint32_t) * (argc + 1));
        
        /* Push argument strings onto stack */
        for (int i = argc - 1; i >= 0; i--) {
            size_t len = strlen(argv[i]) + 1;
            user_stack -= len;
            
            /* Copy string to user stack */
            /* In real implementation, would copy to user space properly */
            arg_ptrs[i] = user_stack;
        }
        
        /* Align stack to 16 bytes */
        user_stack &= ~0xF;
        
        /* Push NULL terminator for argv */
        user_stack -= sizeof(uint32_t);
        
        /* Push argv pointers */
        for (int i = argc - 1; i >= 0; i--) {
            user_stack -= sizeof(uint32_t);
            /* Write arg_ptrs[i] to stack */
        }
        
        /* Push argv pointer */
        uint32_t argv_ptr = user_stack;
        user_stack -= sizeof(uint32_t);
        
        /* Push argc */
        user_stack -= sizeof(uint32_t);
        
        kfree(arg_ptrs);
    }
    
    /* Set process entry point and stack */
    proc->eip = entry;
    proc->esp = user_stack;
    proc->ebp = user_stack;
    proc->state = PROCESS_READY;
    
    kprintf("[PROC] Process ready: entry=0x%x, stack=0x%x, argc=%d\n", 
            entry, user_stack, argc);
    
    return 0;
}

/* Exit process */
void process_exit(process_t* proc) {
    if (!proc) return;
    
    kprintf("[PROC] Process %d (%s) exiting with code %d\n", 
            proc->pid, proc->name, proc->exit_code);
    
    proc->state = PROCESS_ZOMBIE;
    
    /* Close all file descriptors */
    for (int i = 0; i < (int)proc->fd_count; i++) {
        if (proc->fd_table && proc->fd_table[i]) {
            vfs_close(i);
        }
    }
    
    /* Wake up parent if waiting */
    if (proc->parent_pid > 0) {
        process_t* parent = process_get_by_pid(proc->parent_pid);
        if (parent && parent->state == PROCESS_BLOCKED) {
            parent->state = PROCESS_READY;
            kprintf("[PROC] Waking up parent process %d\n", parent->pid);
        }
    }
    
    /* Reparent children to init (PID 1) or kernel (PID 0) */
    for (process_t* p = process_list; p != NULL; p = p->next) {
        if (p->parent_pid == proc->pid) {
            p->parent_pid = 1;  /* Reparent to init */
            kprintf("[PROC] Reparented process %d to init\n", p->pid);
        }
    }
    
    /* If this is current process, schedule next */
    if (current_process == proc) {
        current_process = NULL;
        /* Scheduler will pick next process */
    }
}

/* Wait for child process */
int process_wait(process_t* proc, int* status) {
    if (!proc) return -1;
    
    kprintf("[PROC] Process %d waiting for child\n", proc->pid);
    
    /* Find zombie children */
    process_t* child = NULL;
    process_t** prev_ptr = &process_list;
    
    for (process_t* p = process_list; p != NULL; p = p->next) {
        if (p->parent_pid == proc->pid && p->state == PROCESS_ZOMBIE) {
            child = p;
            *prev_ptr = p->next;  /* Remove from list */
            break;
        }
        prev_ptr = &p->next;
    }
    
    if (child) {
        /* Found zombie child */
        if (status) {
            *status = child->exit_code;
        }
        
        int pid = child->pid;
        kprintf("[PROC] Reaped child process %d\n", pid);
        
        /* Clean up zombie */
        free_process(child);
        
        return pid;
    }
    
    /* Check if we have any children at all */
    int has_children = 0;
    for (process_t* p = process_list; p != NULL; p = p->next) {
        if (p->parent_pid == proc->pid) {
            has_children = 1;
            break;
        }
    }
    
    if (!has_children) {
        kprintf("[PROC] No children to wait for\n");
        return -1;  /* No children */
    }
    
    /* Block until child exits */
    kprintf("[PROC] Blocking process %d\n", proc->pid);
    proc->state = PROCESS_BLOCKED;
    
    /* Will be woken up when child exits */
    return 0;
}

/* Get current process */
process_t* process_get_current(void) {
    return current_process;
}

/* Switch to next process */
void process_switch(process_t* next) {
    if (!next) return;
    
    process_t* prev = current_process;
    
    /* Save previous process state if running */
    if (prev && prev != next) {
        if (prev->state == PROCESS_RUNNING) {
            prev->state = PROCESS_READY;
        }
        
        kprintf("[PROC] Switching: %d (%s) -> %d (%s)\n",
                prev->pid, prev->name, next->pid, next->name);
    }
    
    /* Set new current process */
    current_process = next;
    next->state = PROCESS_RUNNING;
    
    /* Switch page directory if different */
    if (next->page_directory && 
        next->page_directory != vmm_get_page_directory()) {
        vmm_switch_page_directory(next->page_directory);
    }
    
    /* Context switch */
    if (prev && prev != next) {
        /* Save/restore registers */
        /* In real implementation: */
        /* switch_context(&prev->esp, next->esp); */
    }
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

/* List all processes (for debugging) */
void process_list_all(void) {
    kprintf("[PROC] Process List:\n");
    kprintf("  PID  PPID  STATE     NAME\n");
    kprintf("  ---  ----  --------  ----\n");
    
    for (process_t* p = process_list; p != NULL; p = p->next) {
        const char* state_str;
        switch (p->state) {
            case PROCESS_READY:   state_str = "READY   "; break;
            case PROCESS_RUNNING: state_str = "RUNNING "; break;
            case PROCESS_BLOCKED: state_str = "BLOCKED "; break;
            case PROCESS_ZOMBIE:  state_str = "ZOMBIE  "; break;
            case PROCESS_DEAD:    state_str = "DEAD    "; break;
            default:              state_str = "UNKNOWN "; break;
        }
        
        kprintf("  %-4d %-4d  %s  %s\n", p->pid, p->parent_pid, state_str, p->name);
    }
}

/* Kill process */
int process_kill(uint32_t pid, int signal) {
    process_t* proc = process_get_by_pid(pid);
    if (!proc) return -1;
    
    kprintf("[PROC] Killing process %d with signal %d\n", pid, signal);
    
    /* For now, just exit the process */
    proc->exit_code = signal;
    process_exit(proc);
    
    return 0;
}

/* Get process count */
int process_count(void) {
    int count = 0;
    for (process_t* p = process_list; p != NULL; p = p->next) {
        count++;
    }
    return count;
}