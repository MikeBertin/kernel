// kernel/kmain.c — the kernel's C entry point.
//
// The story so far: protected mode (M1), interrupts (M2), and now memory
// management (M3) — a physical frame allocator, paging, and a heap. Each step
// prints what it did so the boot is a narrated tour of the machine coming up.
#include "vga.h"
#include "idt.h"
#include "pit.h"
#include "keyboard.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"

static void ok(const char *label) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  [ok] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts(label);
    vga_putc('\n');
}

void kmain(void) {
    vga_init();

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KERNEL online — protected mode, interrupts, memory.\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // --- M2: interrupts -------------------------------------------------------
    idt_install();
    pit_init(100);
    keyboard_init();
    __asm__ volatile ("sti");
    ok("interrupts: IDT + PIC, timer on IRQ0, keyboard on IRQ1");

    // --- M3: physical memory --------------------------------------------------
    pmm_init();
    vga_puts("  [ok] frames: ");
    vga_put_dec(pmm_used_frames());
    vga_puts(" used / ");
    vga_put_dec(pmm_total_frames());
    vga_puts(" total (");
    vga_put_dec((pmm_total_frames() - pmm_used_frames()) * 4);
    vga_puts(" KiB free)\n");

    // --- M3: paging -----------------------------------------------------------
    paging_init();
    ok("paging enabled (CR0.PG=1) — every address is now virtual");

    // Prove the MMU translates: map a high, unused virtual page to a fresh
    // physical frame, write through the virtual address, and read the value
    // back through the frame's identity-mapped physical address.
    uint32_t phys = pmm_alloc_frame();
    uint32_t virt = 0xE0000000;
    map_page(virt, phys, PAGE_RW);
    *(volatile uint32_t *)virt = 0x0C0FFEE0;
    uint32_t seen = *(volatile uint32_t *)phys;

    vga_puts("  [ok] mapped ");
    vga_put_hex(virt);
    vga_puts(" -> ");
    vga_put_hex(phys);
    vga_puts("; wrote 0x0C0FFEE0, read ");
    vga_put_hex(seen);
    vga_putc('\n');

    // --- M3: heap -------------------------------------------------------------
    heap_init();
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    kfree(a);
    void *c = kmalloc(48);   // should reuse the block just freed at 'a'

    vga_puts("  [ok] kmalloc a=");
    vga_put_hex((uint32_t)a);
    vga_puts(" b=");
    vga_put_hex((uint32_t)b);
    vga_puts("; after kfree(a), c=");
    vga_put_hex((uint32_t)c);
    vga_puts(c == a ? " (reused a)\n" : "\n");

    // --- interactive ----------------------------------------------------------
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nClock ticks on IRQ0 (top-right). Type — keys arrive on IRQ1:\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    for (;;) __asm__ volatile ("hlt");
}
