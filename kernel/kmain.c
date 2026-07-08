// kernel/kmain.c — the kernel's C entry point.
//
// The whole stack, bottom to top: protected mode (M1), interrupts (M2), memory
// management (M3), multitasking (M4), and finally a userspace shell reached
// only through system calls (M5). kmain brings each layer up, then lowers the
// CPU into ring 3 and hands control to the shell.
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "keyboard.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "syscall.h"
#include "shell.h"
#include "fault.h"

extern void enter_usermode(uint32_t eip, uint32_t esp);

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
    vga_puts("KERNEL booting - the whole stack, by hand.\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    gdt_install();     // GDT with user segments + TSS (needed for ring 3)
    idt_install();     // IDT + PIC remap
    syscall_init();    // int 0x80 gate
    fault_init();      // recoverable page-fault handler
    pit_init(100);
    keyboard_init();
    ok("cpu: GDT + TSS, IDT + PIC, syscalls, page-fault handler");

    pmm_init();
    paging_init();     // kernel supervisor-only; just the shell's pages are user
    heap_init();
    ok("memory: paging with ring-3 protection, kernel heap");

    __asm__ volatile ("sti");
    ok("timer on IRQ0, keyboard on IRQ1, interrupts live");

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nDropping to ring 3 - the shell below is unprivileged.\n");
    vga_puts("Try 'poke' to watch it get denied when it touches kernel memory:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Lower the CPU to ring 3 and jump into the shell. This never returns.
    enter_usermode((uint32_t)user_shell,
                   (uint32_t)(user_stack + sizeof(user_stack)));

    for (;;) __asm__ volatile ("hlt");   // unreachable
}
