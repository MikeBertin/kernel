// kernel/gdt.h — rebuild the GDT in C with user-mode segments and a TSS.
//
// The boot sector's GDT only had ring-0 code/data. To run code in ring 3 we
// need ring-3 segments and a Task State Segment: when a ring-3 program traps
// into the kernel (a syscall or an interrupt), the CPU reads the kernel stack
// to switch to from the TSS.
#ifndef GDT_H
#define GDT_H

void gdt_install(void);   // load the new GDT + TSS (kernel and user segments)

#endif
