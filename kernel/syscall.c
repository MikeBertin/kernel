// kernel/syscall.c — dispatch int 0x80 to kernel services.
#include "syscall.h"
#include "isr.h"
#include "vga.h"
#include "keyboard.h"
#include "pit.h"

// Entered (in ring 0) from the int 0x80 stub. The register frame holds the
// ring-3 caller's registers; eax selects the call, ebx carries the argument,
// and we return a value by writing it back into the saved eax.
static void syscall_dispatch(registers_t *r) {
    switch (r->eax) {
        case SYS_PUTC:
            vga_putc((char)r->ebx);
            break;
        case SYS_WRITE:
            vga_puts((const char *)r->ebx);
            break;
        case SYS_READ:
            r->eax = (uint8_t)keyboard_getchar();   // blocks until a key
            break;
        case SYS_CLEAR:
            vga_init();
            break;
        case SYS_UPTIME:
            r->eax = pit_ticks() / 100;             // 100 Hz timer
            break;
        default:
            break;
    }
}

void syscall_init(void) {
    register_interrupt_handler(0x80, syscall_dispatch);
}
