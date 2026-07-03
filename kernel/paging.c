// kernel/paging.c — set up and drive the MMU.
#include "paging.h"
#include "pmm.h"
#include "string.h"

// A page directory and page tables are each 1024 32-bit entries (one 4 KiB
// frame). Entry layout: bits 12-31 = frame address, low bits = flags.
#define ENTRIES 1024
#define IDENTITY_MAP_BYTES (32u * 1024 * 1024)   // identity-map the first 32 MiB

static uint32_t *page_directory;   // physical == virtual (identity-mapped) pointer

static inline void invlpg(uint32_t virt) {
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

// Map one 4 KiB virtual page to a physical frame, allocating a page table for
// this region on demand. Safe both before and after paging is enabled because
// every page table lives in identity-mapped low memory.
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;             // top 10 bits
    uint32_t pt_index = (virt >> 12) & 0x3FF;   // next 10 bits

    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        uint32_t new_pt = pmm_alloc_frame();
        memset((void *)new_pt, 0, FRAME_SIZE);
        // The directory entry must also allow user access, or ring-3 code is
        // blocked from the whole 4 MiB region regardless of the page-table bits.
        page_directory[pd_index] = new_pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    uint32_t *page_table = (uint32_t *)(page_directory[pd_index] & ~0xFFFu);
    page_table[pt_index] = (phys & ~0xFFFu) | (flags & 0xFFF) | PAGE_PRESENT;
    invlpg(virt);
}

// Walk the tables the way the MMU does, to resolve a virtual address.
uint32_t paging_translate(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(page_directory[pd_index] & PAGE_PRESENT)) return 0;
    uint32_t *page_table = (uint32_t *)(page_directory[pd_index] & ~0xFFFu);
    if (!(page_table[pt_index] & PAGE_PRESENT)) return 0;

    return (page_table[pt_index] & ~0xFFFu) | (virt & 0xFFFu);
}

void paging_init(void) {
    page_directory = (uint32_t *)pmm_alloc_frame();
    memset(page_directory, 0, FRAME_SIZE);

    // Identity-map the first 32 MiB: virtual X -> physical X. This keeps the
    // kernel, stack, VGA memory and page tables valid across the switch.
    // PAGE_USER lets our ring-3 shell (M5) execute here too. (A production
    // kernel would give userspace its own private, protected mappings.)
    for (uint32_t addr = 0; addr < IDENTITY_MAP_BYTES; addr += FRAME_SIZE)
        map_page(addr, addr, PAGE_RW | PAGE_USER);

    // Point CR3 at the directory, then set the paging bit (CR0.PG). The very
    // next instruction fetch already goes through the MMU.
    __asm__ volatile ("mov %0, %%cr3" : : "r"(page_directory));
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;   // CR0.PG
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}
