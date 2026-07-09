// kernel/sched.h — a round-robin pre-emptive scheduler.
//
// Tasks are just a saved stack pointer and a link to the next task, arranged in
// a ring. On every timer tick the running task is frozen and the next one runs
// — pre-emptive multitasking, no cooperation required from the tasks.
#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

typedef void (*task_entry_t)(void);

void sched_init(void);                                  // create the idle task
void task_create(const char *name, task_entry_t entry); // ring-0 task (M4)
void task_create_user(const char *name, task_entry_t entry, int id); // ring-3 process (M5.2)
void sched_start(void);                                 // begin pre-empting on the timer
void schedule(void);                                    // switch to the next task

int       sched_current_id(void);        // id of the running process
uint32_t *sched_current_pagedir(void);   // its address space

#endif
