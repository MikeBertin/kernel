// kernel/kmain.c — the kernel's C entry point.
//
// The whole stack, bottom to top: protected mode (M1), interrupts (M2), memory
// management (M3), pre-emptive multitasking (M4), userspace + syscalls (M5),
// real memory protection (M5.1), several *isolated* user processes running at
// once (M5.2), and an interactive ring-3 shell as one of them (M5.3).
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
#include "sched.h"

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
    fault_init();      // page-fault handler
    pit_init(100);
    keyboard_init();
    ok("cpu: GDT + TSS, IDT + PIC, syscalls, page-fault handler");

    pmm_init();
    paging_init();     // kernel supervisor-only; paging on
    heap_init();
    ok("memory: paging with ring-3 protection, kernel heap");

    __asm__ volatile ("sti");
    ok("timer on IRQ0, interrupts live");

    // M5.2: three ring-3 worker processes, each in its OWN address space.
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nScheduler: 4 isolated user processes, each with its own page directory.\n");
    vga_puts("Same virtual address 0xB0000000 -> a different physical frame each:\n");

    // M5.3: the fourth process is the shell. Its output scrolls in the bottom
    // window (rows 14-24) while the workers keep reporting on rows 9-11.
    vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
    vga_puts_at(13, 0, "-- process 3: a shell in ring 3. Type help, then try poke ---------------------");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_set_window(14, 24);

    sched_init();
    task_create_user("worker", user_worker, 0);
    task_create_user("worker", user_worker, 1);
    task_create_user("worker", user_worker, 2);
    task_create_user("shell", user_shell, SHELL_PID);
    sched_start();     // the timer now pre-empts between the four processes

    // The idle task: sleep until the next interrupt. It's scheduled in turn too.
    for (;;) __asm__ volatile ("hlt");
}
