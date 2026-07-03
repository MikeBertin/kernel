// kernel/kmain.c — the kernel's C entry point.
//
// By the time we get here the CPU is in 32-bit protected mode with a flat GDT,
// A20 is on, and we have a stack. This is the first C code in the OS. It runs
// with no standard library, no heap, no OS underneath it — only what we wrote.
#include "vga.h"

void kmain(void) {
    vga_init();

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KERNEL: now in 32-bit protected mode.\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("This is C, freestanding, writing straight to VGA memory at 0xB8000.\n");
    vga_puts("Flat GDT installed, A20 enabled, BIOS left behind.\n");
    vga_puts("No standard library. No OS beneath us. The metal is ours.\n");

    // kmain returns into the entry stub's hlt-loop.
}
