// kernel/syscall.h — the system-call boundary.
//
// A syscall is the one guarded doorway from an unprivileged program into the
// kernel. Ring-3 code puts a call number in EAX (and args in EBX/ECX/...) and
// executes `int 0x80`; the CPU jumps — via a gate we deliberately made callable
// from ring 3 — into the kernel, which does the privileged work on its behalf.
#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_PUTC   0   // ebx = char
#define SYS_WRITE  1   // ebx = char*
#define SYS_READ   2   // -> eax = char (blocks)
#define SYS_CLEAR  3
#define SYS_UPTIME 4   // -> eax = seconds since boot
#define SYS_REPORT 5   // print this process's private counter + its physical frame

void syscall_init(void);   // register the int 0x80 handler

#endif
