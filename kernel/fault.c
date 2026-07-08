// kernel/fault.c — catch page faults; recover from ring-3 ones.
//
// When a program violates its memory permissions the CPU raises a page fault
// (vector 14) with the offending address in CR2. A real OS doesn't die — it
// kills the offending process and carries on. With a single process (the shell)
// "killing" it means resetting it to a fresh stack at its command loop.
#include "fault.h"
#include "isr.h"
#include "vga.h"
#include "shell.h"

static void page_fault(registers_t *r) {
    uint32_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    // A fault from ring 3 (CS privilege bits == 3) is a userspace access
    // violation: report it and restart the shell rather than halting.
    if ((r->cs & 3) == 3) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("\n[page fault] ring 3 denied access to ");
        vga_put_hex(cr2);
        vga_puts(" - that's kernel memory. Process killed; restarting shell.\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        // Rewrite the interrupt frame so iret resumes the shell's command loop
        // on a fresh stack — the CPU returns to ring 3 (cs/ss/eflags unchanged).
        r->eip     = (uint32_t)shell_loop;
        r->useresp = (uint32_t)(user_stack + USER_STACK_SIZE);
        return;
    }

    // A fault from ring 0 is a genuine kernel bug — nowhere safe to recover to.
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_puts("\n[KERNEL PAGE FAULT] cr2=");
    vga_put_hex(cr2);
    vga_puts(" eip=");
    vga_put_hex(r->eip);
    vga_puts(" - halted.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

void fault_init(void) {
    register_interrupt_handler(14, page_fault);
}
