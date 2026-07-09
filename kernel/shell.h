// kernel/shell.h — a tiny interactive shell that runs in ring 3.
#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

#define USER_STACK_SIZE 16384

void user_shell(void);   // ring-3 entry: banner, then the command loop
void shell_loop(void);   // the command loop alone (re-entered after a fault)
void user_worker(void);  // ring-3 worker program (M5.2 isolation demo)

// Page-aligned so paging can mark exactly its pages user-accessible.
extern uint8_t user_stack[USER_STACK_SIZE];

#endif
