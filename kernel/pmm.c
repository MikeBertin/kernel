// kernel/pmm.c — bitmap physical frame allocator.
#include "pmm.h"

// We manage the first 32 MiB of RAM (matches the memory we hand QEMU/v86).
#define MANAGED_MEMORY (32u * 1024 * 1024)
#define TOTAL_FRAMES   (MANAGED_MEMORY / FRAME_SIZE)   // 8192 frames

// One bit per frame: 1 = used, 0 = free.
static uint32_t frame_bitmap[TOTAL_FRAMES / 32];
static uint32_t used_frames;

// The linker places this symbol at the end of the loaded kernel image.
extern uint8_t kernel_end[];

static void set_used(uint32_t frame)  { frame_bitmap[frame / 32] |=  (1u << (frame % 32)); }
static void set_free(uint32_t frame)  { frame_bitmap[frame / 32] &= ~(1u << (frame % 32)); }
static int  is_used(uint32_t frame)   { return frame_bitmap[frame / 32] &   (1u << (frame % 32)); }

void pmm_init(void) {
    // Start with everything free...
    for (uint32_t i = 0; i < TOTAL_FRAMES / 32; i++) frame_bitmap[i] = 0;
    used_frames = 0;

    // ...then reserve everything from address 0 up to the end of the kernel:
    // the low 1 MiB (BIOS, video, our boot sector) and the kernel image itself
    // must never be handed out.
    uint32_t reserved_end = (uint32_t)kernel_end;
    uint32_t last = (reserved_end + FRAME_SIZE - 1) / FRAME_SIZE;   // round up
    for (uint32_t f = 0; f < last && f < TOTAL_FRAMES; f++) {
        set_used(f);
        used_frames++;
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < TOTAL_FRAMES; f++) {
        if (!is_used(f)) {
            set_used(f);
            used_frames++;
            return f * FRAME_SIZE;
        }
    }
    return 0;   // out of physical memory
}

void pmm_free_frame(uint32_t addr) {
    uint32_t f = addr / FRAME_SIZE;
    if (f < TOTAL_FRAMES && is_used(f)) {
        set_free(f);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return TOTAL_FRAMES; }
uint32_t pmm_used_frames(void)  { return used_frames; }
