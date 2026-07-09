// kernel/gdt.h — rebuild the GDT in C with user-mode segments and a TSS.
//
// The boot sector's GDT only had ring-0 code/data. To run code in ring 3 we
// need ring-3 segments and a Task State Segment: when a ring-3 program traps
// into the kernel (a syscall or an interrupt), the CPU reads the kernel stack
// to switch to from the TSS.
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

void gdt_install(void);   // load the new GDT + TSS (kernel and user segments)

// Set the ring-0 stack the CPU switches to on a trap from ring 3. The scheduler
// updates this on every switch so each process traps onto its own kernel stack.
void tss_set_kernel_stack(uint32_t esp0);

#endif
