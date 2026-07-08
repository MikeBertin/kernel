// kernel/shell.c — a ring-3 shell.
//
// Everything here runs unprivileged. It cannot call kernel functions or touch
// hardware directly; its only channel to the kernel is `int 0x80`. The thin
// wrappers below are the entire interface — read a key, write output, etc.
#include "shell.h"
#include "syscall.h"

// The ring-3 stack. Page-aligned and a whole number of pages, so paging can
// mark exactly these pages user-accessible without exposing neighbouring
// kernel BSS.
uint8_t user_stack[USER_STACK_SIZE] __attribute__((aligned(4096)));

// --- syscall wrappers (the only privileged operations available to us) ---
static void s_putc(char c) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_PUTC), "b"((uint32_t)(uint8_t)c) : "memory");
}
static void s_write(const char *str) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_WRITE), "b"(str) : "memory");
}
static char s_read(void) {
    uint32_t c;
    __asm__ volatile ("int $0x80" : "=a"(c) : "a"(SYS_READ) : "memory");
    return (char)c;
}
static void s_clear(void) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_CLEAR) : "memory");
}
static uint32_t s_uptime(void) {
    uint32_t v;
    __asm__ volatile ("int $0x80" : "=a"(v) : "a"(SYS_UPTIME) : "memory");
    return v;
}

// --- tiny self-contained string helpers (ring 3 has no libc either) ---
static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static int starts_with(const char *s, const char *prefix) {
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}
static void s_write_dec(uint32_t v) {
    char buf[11];
    int i = 0;
    if (!v) { s_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + v % 10); v /= 10; }
    while (i) s_putc(buf[--i]);
}

static void run(const char *line) {
    if (streq(line, "help"))
        s_write("commands: help, echo <text>, uptime, clear, poke\n");
    else if (streq(line, "clear"))
        s_clear();
    else if (streq(line, "uptime")) {
        s_write("up ");
        s_write_dec(s_uptime());
        s_write(" seconds\n");
    } else if (starts_with(line, "echo ")) {
        s_write(line + 5);
        s_putc('\n');
    } else if (streq(line, "poke")) {
        // Deliberately reach past our sandbox: write to VGA memory (0xB8000),
        // a kernel-only page. In ring 3 this page-faults — the kernel catches
        // it, "kills" us, and restarts the shell. Real memory protection.
        s_write("writing directly to kernel memory (0xB8000)...\n");
        *(volatile unsigned int *)0xB8000 = 0x2F212F21;
        s_write("...if you can read this, protection failed.\n");
    } else if (line[0] != '\0')
        s_write("unknown command (try 'help')\n");
}

void shell_loop(void) {
    char line[128];
    for (;;) {
        s_write("$ ");
        int n = 0;
        for (;;) {
            char c = s_read();
            if (c == '\n' || c == '\r') { s_putc('\n'); break; }
            if (c == '\b') { if (n > 0) { n--; s_write("\b \b"); } continue; }
            if (c >= 32 && c < 127 && n < 127) { line[n++] = c; s_putc(c); }
        }
        line[n] = '\0';
        run(line);
    }
}

void user_shell(void) {
    s_write("\nKERNEL shell — you are in ring 3. Every action is a syscall.\n");
    s_write("commands: help, echo <text>, uptime, clear, poke\n\n");
    shell_loop();
}
