// kernel/kmain.c — the kernel's C entry point.
//
// M2: the CPU can now call our code asynchronously. We build an interrupt
// table, remap the interrupt controller, and attach a timer and a keyboard —
// then enable interrupts and let the hardware drive us.
#include "vga.h"
#include "idt.h"
#include "pit.h"
#include "keyboard.h"

void kmain(void) {
    vga_init();

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KERNEL: 32-bit protected mode.\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Installing IDT, remapping the PIC, wiring timer + keyboard...\n");

    idt_install();     // build the IDT and remap the PIC (IRQs -> vectors 32-47)
    pit_init(100);     // 100 Hz heartbeat on IRQ0
    keyboard_init();   // echo keystrokes on IRQ1

    __asm__ volatile ("sti");   // unmask interrupts — the machine comes alive

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nInterrupts live. The clock (top-right) ticks on IRQ0.\n");
    vga_puts("Type — each keystroke is an IRQ1 handled by our driver:\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Idle: sleep until the next interrupt. Everything now happens in handlers.
    for (;;) __asm__ volatile ("hlt");
}
