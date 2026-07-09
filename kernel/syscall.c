// kernel/syscall.c — dispatch int 0x80 to kernel services.
#include "syscall.h"
#include "isr.h"
#include "vga.h"
#include "keyboard.h"
#include "pit.h"
#include "sched.h"
#include "paging.h"

#define UDATA_VA 0xB0000000u   // each process's private data page: [id, counter]

static char *put_str(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *put_dec(char *p, uint32_t v) {
    char t[11]; int i = 0;
    if (!v) { *p++ = '0'; return p; }
    while (v) { t[i++] = (char)('0' + v % 10); v /= 10; }
    while (i) *p++ = t[--i];
    return p;
}
static char *put_hex(char *p, uint32_t v) {
    static const char h[] = "0123456789ABCDEF";
    *p++ = '0'; *p++ = 'x';
    for (int s = 28; s >= 0; s -= 4) *p++ = h[(v >> s) & 0xF];
    return p;
}

// A process asks the kernel to report its private counter. We read it from the
// process's own data page (mapped in the currently-active address space) and
// translate that virtual address to physical — printing both proves that each
// process's identical virtual address is backed by a *different* frame.
static void sys_report(void) {
    int id = sched_current_id();
    uint32_t counter = *(volatile uint32_t *)(UDATA_VA + 4);
    uint32_t phys = paging_translate_in(sched_current_pagedir(), UDATA_VA);

    char line[80], *p = line;
    p = put_str(p, "proc ");
    p = put_dec(p, (uint32_t)id);
    p = put_str(p, ": counter=");
    p = put_dec(p, counter);
    p = put_str(p, "   data 0xB0000000 -> phys ");
    p = put_hex(p, phys);
    while (p < line + 78) *p++ = ' ';   // pad to clear any prior longer line
    *p = '\0';

    uint8_t saved = vga_get_color();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts_at((size_t)(12 + id), 2, line);
    vga_set_color_raw(saved);
}

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
        case SYS_REPORT:
            sys_report();
            break;
        default:
            break;
    }
}

void syscall_init(void) {
    register_interrupt_handler(0x80, syscall_dispatch);
}
