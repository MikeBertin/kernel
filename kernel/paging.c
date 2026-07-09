// kernel/paging.c — set up and drive the MMU, incl. per-process address spaces.
#include "paging.h"
#include "pmm.h"
#include "string.h"
#include "shell.h"

// Marked by the linker: the ring-3 shell's / user programs' code + rodata.
extern uint8_t user_text_start[], user_text_end[];

#define ENTRIES 1024
#define IDENTITY_MAP_BYTES (32u * 1024 * 1024)   // identity-map the first 32 MiB

// The kernel's own address space. Every process address space is a clone of
// this (sharing the kernel's page tables) plus its own private user pages.
static uint32_t *kernel_directory;

static inline void invlpg(uint32_t virt) {
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

// Map one 4 KiB page into a *specific* page directory, allocating a page table
// for the region on demand. Works on any directory as long as the currently
// active CR3 identity-maps low memory (where page tables live) — true during
// setup, since every directory shares the kernel's identity map.
void map_page_in(uint32_t *pd, uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(pd[pd_index] & PAGE_PRESENT)) {
        uint32_t new_pt = pmm_alloc_frame();
        memset((void *)new_pt, 0, FRAME_SIZE);
        pd[pd_index] = new_pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }
    uint32_t *page_table = (uint32_t *)(pd[pd_index] & ~0xFFFu);
    page_table[pt_index] = (phys & ~0xFFFu) | (flags & 0xFFF) | PAGE_PRESENT;
    invlpg(virt);
}

// Map into the kernel directory (used during boot-time setup).
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    map_page_in(kernel_directory, virt, phys, flags);
}

uint32_t paging_translate_in(uint32_t *pd, uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    if (!(pd[pd_index] & PAGE_PRESENT)) return 0;
    uint32_t *page_table = (uint32_t *)(pd[pd_index] & ~0xFFFu);
    if (!(page_table[pt_index] & PAGE_PRESENT)) return 0;
    return (page_table[pt_index] & ~0xFFFu) | (virt & 0xFFFu);
}

uint32_t paging_translate(uint32_t virt) {
    return paging_translate_in(kernel_directory, virt);
}

// Create a fresh address space: a new page directory that shares the kernel's
// mappings (so traps into the kernel keep working) but into which we can add
// private user pages that no other process can see.
uint32_t *paging_new_address_space(void) {
    uint32_t *pd = (uint32_t *)pmm_alloc_frame();
    for (int i = 0; i < ENTRIES; i++)
        pd[i] = kernel_directory[i];   // share the kernel's page tables
    return pd;
}

void paging_switch(uint32_t *pd) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pd));   // reload CR3 (flushes the TLB)
}

uint32_t *paging_kernel_directory(void) {
    return kernel_directory;
}

void paging_init(void) {
    kernel_directory = (uint32_t *)pmm_alloc_frame();
    memset(kernel_directory, 0, FRAME_SIZE);

    // Identity-map the first 32 MiB as SUPERVISOR-only. Ring 3 cannot touch it.
    for (uint32_t addr = 0; addr < IDENTITY_MAP_BYTES; addr += FRAME_SIZE)
        map_page(addr, addr, PAGE_RW);

    // The user programs' shared code + rodata (.user_text) must be executable
    // from ring 3 in every address space; it lives in the shared kernel region.
    for (uint32_t a = (uint32_t)user_text_start & ~0xFFFu; a < (uint32_t)user_text_end; a += FRAME_SIZE)
        map_page(a, a, PAGE_RW | PAGE_USER);

    // Turn paging on with the kernel directory active.
    paging_switch(kernel_directory);
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;   // CR0.PG
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}
