// kernel/sched.h — a round-robin pre-emptive scheduler.
//
// Tasks are just a saved stack pointer and a link to the next task, arranged in
// a ring. On every timer tick the running task is frozen and the next one runs
// — pre-emptive multitasking, no cooperation required from the tasks.
#ifndef SCHED_H
#define SCHED_H

typedef void (*task_entry_t)(void);

void sched_init(void);                                  // create the idle task
void task_create(const char *name, task_entry_t entry); // add a task to the ring
void sched_start(void);                                 // begin pre-empting on the timer
void schedule(void);                                    // switch to the next task

#endif
