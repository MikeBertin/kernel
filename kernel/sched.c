// kernel/sched.c — round-robin scheduler.
#include "sched.h"
#include "heap.h"
#include "pit.h"
#include <stdint.h>

#define STACK_SIZE 4096

typedef struct task {
    uint32_t     esp;    // saved kernel stack pointer (the whole context lives here)
    struct task *next;   // next task in the ring
    const char  *name;
} task_t;

extern void context_switch(uint32_t *save_old_esp, uint32_t new_esp);
extern void task_trampoline(void);

static task_t *current;   // the running task
static task_t *tail;      // where new tasks are appended

// Create the "idle" task that represents the kernel's own current context. Its
// esp is filled in the first time we switch away from it.
void sched_init(void) {
    task_t *idle = kmalloc(sizeof(task_t));
    idle->name = "idle";
    idle->next = idle;    // a ring of one
    current = idle;
    tail = idle;
}

void task_create(const char *name, task_entry_t entry) {
    task_t  *t     = kmalloc(sizeof(task_t));
    uint8_t *stack = kmalloc(STACK_SIZE);
    t->name = name;

    // Craft an initial stack so the first context_switch into this task lands in
    // task_trampoline with EBX = entry. Layout must mirror context_switch's
    // restore sequence (pop edi, esi, ebx, ebp; ret).
    uint32_t *sp = (uint32_t *)(stack + STACK_SIZE);
    *(--sp) = (uint32_t)task_trampoline;   // ret target
    *(--sp) = 0;                           // ebp
    *(--sp) = (uint32_t)entry;             // ebx -> passed to the trampoline
    *(--sp) = 0;                           // esi
    *(--sp) = 0;                           // edi
    t->esp = (uint32_t)sp;

    // Append to the ring (interrupts are still off / scheduler not started).
    t->next = current;
    tail->next = t;
    tail = t;
}

// Called from the timer IRQ. Freeze the current task, advance the ring, and
// switch. context_switch returns here only when this task is later resumed.
void schedule(void) {
    task_t *prev = current;
    current = current->next;
    if (prev != current)
        context_switch(&prev->esp, current->esp);
}

void sched_start(void) {
    pit_set_tick_hook(schedule);   // let the timer drive pre-emption
}
