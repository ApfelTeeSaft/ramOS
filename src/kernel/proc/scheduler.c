/* scheduler.c - Simple round-robin scheduler */

#include "scheduler.h"
#include "../core/console.h"

#define MAX_PROCESSES 64

static process_t* ready_queue[MAX_PROCESSES];
static int queue_size = 0;
static int current_index = 0;

/* Initialize scheduler */
void scheduler_init(void) {
    kprintf("[SCHED] Initializing scheduler...\n");
    queue_size = 0;
    current_index = 0;
}

/* Add process to ready queue */
void scheduler_add(process_t* proc) {
    if (!proc || queue_size >= MAX_PROCESSES) return;
    
    ready_queue[queue_size++] = proc;
}

/* Remove process from ready queue */
void scheduler_remove(process_t* proc) {
    if (!proc) return;
    
    for (int i = 0; i < queue_size; i++) {
        if (ready_queue[i] == proc) {
            /* Shift remaining processes */
            for (int j = i; j < queue_size - 1; j++) {
                ready_queue[j] = ready_queue[j + 1];
            }
            queue_size--;
            
            /* Adjust current index if needed */
            if (current_index >= queue_size && queue_size > 0) {
                current_index = 0;
            }
            break;
        }
    }
}

/* Schedule next process (round-robin) */
void scheduler_schedule(void) {
    if (queue_size == 0) return;
    
    /* Move to next process */
    current_index = (current_index + 1) % queue_size;
    
    process_t* next = ready_queue[current_index];
    if (next && next->state == PROCESS_READY) {
        process_switch(next);
    }
}

/* Yield CPU voluntarily */
void scheduler_yield(void) {
    scheduler_schedule();
}