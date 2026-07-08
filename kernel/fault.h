// kernel/fault.h — a recoverable page-fault handler.
#ifndef FAULT_H
#define FAULT_H

void fault_init(void);   // install the page-fault (vector 14) handler

#endif
