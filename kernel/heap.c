// kernel/heap.c — first-fit heap with splitting and coalescing.
#include "heap.h"
#include <stdint.h>

#define HEAP_SIZE (256u * 1024)   // 256 KiB arena, lives in BSS (zeroed at boot)

// Each block is [ header | payload ]. Blocks are laid out contiguously, so the
// next block always starts at (this header + sizeof(header) + size).
typedef struct block {
    size_t        size;    // payload bytes
    int           free;    // 1 = available
    struct block *next;    // next block in address order
} block_t;

static uint8_t heap[HEAP_SIZE];
static block_t *head;

void heap_init(void) {
    head = (block_t *)heap;
    head->size = HEAP_SIZE - sizeof(block_t);
    head->free = 1;
    head->next = NULL;
}

// If a free block is much bigger than requested, carve the tail into a new
// free block so the surplus stays usable.
static void split(block_t *b, size_t size) {
    if (b->size < size + sizeof(block_t) + 8) return;   // not worth splitting
    block_t *rest = (block_t *)((uint8_t *)b + sizeof(block_t) + size);
    rest->size = b->size - size - sizeof(block_t);
    rest->free = 1;
    rest->next = b->next;
    b->size = size;
    b->next = rest;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~((size_t)7);   // 8-byte align

    for (block_t *b = head; b; b = b->next) {
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            return (uint8_t *)b + sizeof(block_t);
        }
    }
    return NULL;   // out of heap
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    b->free = 1;

    // Coalesce with the next block if it is also free (keeps fragmentation down).
    if (b->next && b->next->free) {
        b->size += sizeof(block_t) + b->next->size;
        b->next = b->next->next;
    }
}
