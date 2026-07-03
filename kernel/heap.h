// kernel/heap.h — a simple kernel heap (kmalloc / kfree).
//
// A first-fit allocator over a fixed arena. Each block carries a small header;
// kmalloc splits a big free block, kfree marks it free and coalesces with the
// following block so the heap doesn't fragment into confetti.
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void  heap_init(void);
void *kmalloc(size_t size);
void  kfree(void *ptr);

#endif
