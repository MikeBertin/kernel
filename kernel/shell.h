// kernel/shell.h — a tiny interactive shell that runs in ring 3.
#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

void user_shell(void);          // the ring-3 entry point (syscalls only)
extern uint8_t user_stack[16384];

#endif
