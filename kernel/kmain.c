// kernel/kmain.c — the kernel's C entry point.
//
// The story so far: protected mode (M1), interrupts (M2), memory management
// (M3), and now pre-emptive multitasking (M4). Each step narrates itself; then
// three tasks run "at once", time-sliced by the timer.
#include "vga.h"
#include "idt.h"
#include "pit.h"
#include "keyboard.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "sched.h"

static void ok(const char *label) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  [ok] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts(label);
    vga_putc('\n');
}

static void utoa(uint32_t v, char *b) {
    char t[11];
    int i = 0;
    if (!v) { b[0] = '0'; b[1] = 0; return; }
    while (v) { t[i++] = (char)('0' + v % 10); v /= 10; }
    int j = 0;
    while (i) b[j++] = t[--i];
    b[j] = 0;
}

// A worker task: spin forever incrementing a counter and redrawing it in place.
// The task never yields — only the timer pre-empting it lets the others run, so
// three counters climbing together is proof of pre-emptive multitasking.
static void worker(size_t row, const char *name) {
    static const char spin[4] = {'|', '/', '-', '\\'};
    uint32_t n = 0;
    for (;;) {
        n++;
        char buf[48];
        int p = 0;
        for (const char *s = name; *s; s++) buf[p++] = *s;
        buf[p++] = ':'; buf[p++] = ' ';
        char num[11];
        utoa(n, num);
        for (int k = 0; num[k]; k++) buf[p++] = num[k];
        buf[p++] = ' '; buf[p++] = spin[(n >> 3) & 3]; buf[p] = 0;

        vga_puts_at(row, 4, buf);
        for (volatile uint32_t d = 0; d < 400000; d++) { /* burn time */ }
    }
}

static void task_a(void) { worker(11, "task A"); }
static void task_b(void) { worker(12, "task B"); }
static void task_c(void) { worker(13, "task C"); }

void kmain(void) {
    vga_init();

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("KERNEL online — protected mode, interrupts, memory, tasks.\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // M2: interrupts
    idt_install();
    pit_init(100);
    keyboard_init();
    __asm__ volatile ("sti");
    ok("interrupts: IDT + PIC, timer on IRQ0, keyboard on IRQ1");

    // M3: physical memory, paging, heap
    pmm_init();
    paging_init();
    heap_init();
    ok("memory: frame allocator, paging (CR0.PG=1), kernel heap");

    // M4: three tasks, pre-empted by the timer
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nScheduler: three tasks time-sliced by the timer (IRQ0):\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    sched_init();
    task_create("task A", task_a);
    task_create("task B", task_b);
    task_create("task C", task_c);
    sched_start();   // pre-emption begins on the next tick

    vga_set_cursor(15, 0);
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("Type here — keys echo on IRQ1 while the tasks run above:\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // The idle task: sleep until an interrupt. It, too, is scheduled in turn.
    for (;;) __asm__ volatile ("hlt");
}
