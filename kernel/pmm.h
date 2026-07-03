// kernel/pmm.h — physical memory manager (frame allocator).
//
// Physical RAM is handed out in 4 KiB "frames". A bitmap tracks which frames
// are free. This is the bedrock every higher allocator sits on: page tables
// and (indirectly) the heap all come from here.
#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define FRAME_SIZE 4096u

void     pmm_init(void);            // build the bitmap, reserve low mem + kernel
uint32_t pmm_alloc_frame(void);     // returns a free physical frame address, or 0
void     pmm_free_frame(uint32_t addr);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);

#endif
