// kernel/paging.h — virtual memory via x86 two-level paging.
//
// With paging on, every address the CPU touches is *virtual*: the MMU walks a
// page directory -> page table to find the real physical frame. We identity-map
// the low memory first (so the running kernel doesn't vanish the instant paging
// switches on), then can map any virtual page to any physical frame we like.
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

void paging_init(void);                                    // identity-map + enable
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
uint32_t paging_translate(uint32_t virt);                  // virt -> phys (0 if unmapped)

#endif
