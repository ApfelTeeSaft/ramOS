/* scheduler.h - Task scheduler */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

/* Initialize scheduler */
void scheduler_init(void);

/* Add process to scheduler */
void scheduler_add(process_t* proc);

/* Remove process from scheduler */
void scheduler_remove(process_t* proc);

/* Schedule next process (called from timer interrupt) */
void scheduler_schedule(void);

/* Yield CPU to next process */
void scheduler_yield(void);

#endif /* SCHEDULER_H */