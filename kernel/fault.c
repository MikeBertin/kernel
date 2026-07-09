// kernel/fault.c — page-fault handler.
//
// Reports the faulting address (CR2) and whether the fault came from a user
// process (ring 3) or the kernel (ring 0), then halts. (Earlier milestones
// recovered a single ring-3 shell here; with several isolated processes the
// clean, debuggable behaviour is to report and stop.)
#include "fault.h"
#include "isr.h"
#include "vga.h"
#include "sched.h"

static void page_fault(registers_t *r) {
    uint32_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    vga_set_color(VGA_WHITE, VGA_RED);
    if ((r->cs & 3) == 3) {
        vga_puts("\n[page fault] user process ");
        vga_put_dec((uint32_t)sched_current_id());
        vga_puts(" touched ");
    } else {
        vga_puts("\n[KERNEL PAGE FAULT] ");
    }
    vga_put_hex(cr2);
    vga_puts(" (eip=");
    vga_put_hex(r->eip);
    vga_puts(", err=");
    vga_put_dec(r->err_code);
    vga_puts(") - halted.\n");

    for (;;) __asm__ volatile ("cli; hlt");
}

void fault_init(void) {
    register_interrupt_handler(14, page_fault);
}
