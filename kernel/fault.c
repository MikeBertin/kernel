// kernel/fault.c — page-fault handler.
//
// Reports the faulting address (CR2) and whether the fault came from a user
// process (ring 3) or the kernel (ring 0). If the shell was the culprit (its
// `poke` command does it on purpose) the kernel restarts the shell on a fresh
// stack and every other process carries on. Any other fault halts.
#include "fault.h"
#include "isr.h"
#include "vga.h"
#include "sched.h"
#include "shell.h"

static void page_fault(registers_t *r) {
    uint32_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    if ((r->cs & 3) == 3 && sched_current_id() == SHELL_PID) {
        uint8_t saved = vga_get_color();
        vga_set_color(VGA_WHITE, VGA_RED);
        vga_puts("[page fault] the CPU blocked process 3 at ");
        vga_put_hex(cr2);
        vga_set_color_raw(saved);
        vga_puts("\nshell restarted; the other processes never noticed.\n");
        // Rewrite the saved ring-3 frame: iret resumes the shell's command loop
        // on a fresh user stack instead of retrying the faulting write.
        r->eip = (uint32_t)shell_loop;
        r->useresp = USER_STACK_TOP;
        return;
    }

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
