// kernel/sched.c — round-robin scheduler with isolated userspace processes.
#include "sched.h"
#include "heap.h"
#include "pit.h"
#include "pmm.h"
#include "paging.h"
#include "gdt.h"
#include <stdint.h>

#define KSTACK_SIZE  8192
#define USTACK_TOP   0xC0000000u   // top of each process's private user stack
#define USTACK_PAGES 4
#define UDATA_VA     0xB0000000u   // each process's private data page: [id, counter]

typedef struct task {
    uint32_t   esp;          // saved kernel-side stack pointer (the whole context)
    uint32_t  *page_dir;     // this task's address space (CR3 value)
    uint32_t   kstack_top;   // top of its ring-0 stack (loaded into tss.esp0)
    int        id;
    struct task *next;
    const char *name;
} task_t;

extern void context_switch(uint32_t *save_old_esp, uint32_t new_esp);
extern void user_task_trampoline(void);

static task_t *current;
static task_t *tail;

// The idle task = the kernel's own context. It stays in ring 0 (never traps
// from userspace), and runs in the kernel address space.
void sched_init(void) {
    task_t *idle = kmalloc(sizeof(task_t));
    idle->name = "idle";
    idle->id = -1;
    idle->page_dir = paging_kernel_directory();
    idle->kstack_top = 0;         // unused: idle never traps from ring 3
    idle->next = idle;
    current = idle;
    tail = idle;
}

// Create a ring-3 process with its OWN address space: shared kernel + shared
// user code, but a private stack and a private data page. Two processes can use
// the very same virtual address and never see each other's memory.
void task_create_user(const char *name, task_entry_t entry, int id) {
    task_t *t = kmalloc(sizeof(task_t));
    t->name = name;
    t->id = id;
    t->page_dir = paging_new_address_space();

    // Private user stack (mapped only in this address space).
    for (int i = 1; i <= USTACK_PAGES; i++) {
        uint32_t frame = pmm_alloc_frame();
        map_page_in(t->page_dir, USTACK_TOP - i * FRAME_SIZE, frame, PAGE_RW | PAGE_USER);
    }

    // Private data page, pre-loaded with [id, counter=0]. Written here via its
    // physical address (identity-mapped under the kernel directory we're on).
    uint32_t data = pmm_alloc_frame();
    ((volatile uint32_t *)data)[0] = (uint32_t)id;
    ((volatile uint32_t *)data)[1] = 0;
    map_page_in(t->page_dir, UDATA_VA, data, PAGE_RW | PAGE_USER);

    // Ring-0 stack + a crafted context so the first switch lands in
    // user_task_trampoline (which iret's to ring 3). Layout must match
    // context_switch's restore order (pop edi,esi,ebx,ebp; ret), with the
    // trampoline's two "arguments" sitting just above the return address.
    uint8_t *kstack = kmalloc(KSTACK_SIZE);
    t->kstack_top = (uint32_t)(kstack + KSTACK_SIZE);

    uint32_t *sp = (uint32_t *)t->kstack_top;
    *(--sp) = USTACK_TOP;                       // user esp   (trampoline arg 2)
    *(--sp) = (uint32_t)entry;                  // user eip   (trampoline arg 1)
    *(--sp) = (uint32_t)user_task_trampoline;   // context_switch return address
    *(--sp) = 0;   // ebp
    *(--sp) = 0;   // ebx
    *(--sp) = 0;   // esi
    *(--sp) = 0;   // edi
    t->esp = (uint32_t)sp;

    t->next = current;
    tail->next = t;
    tail = t;
}

// Called from the timer IRQ. Point the CPU at the next task's kernel stack and
// address space, then switch kernel contexts.
void schedule(void) {
    task_t *prev = current;
    current = current->next;
    if (prev == current) return;

    tss_set_kernel_stack(current->kstack_top);   // where ring-3 traps will land
    paging_switch(current->page_dir);            // its address space (CR3)
    context_switch(&prev->esp, current->esp);    // its kernel context
}

void sched_start(void) {
    pit_set_tick_hook(schedule);
}

// Accessors for the syscall layer.
int       sched_current_id(void)      { return current->id; }
uint32_t *sched_current_pagedir(void) { return current->page_dir; }
