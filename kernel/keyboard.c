// kernel/keyboard.c — translate IRQ1 scancodes and buffer them for readers.
#include "keyboard.h"
#include "isr.h"
#include "io.h"

#define KBD_DATA 0x60
#define KBUF_SIZE 128

// Scancode set 1, US QWERTY. Index = make code; value = ASCII (0 = ignore).
static const char scancode_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
};

// A small ring buffer filled by the interrupt, drained by keyboard_getchar.
static volatile char kbuf[KBUF_SIZE];
static volatile int  khead, ktail;

static void keyboard_callback(registers_t *r) {
    (void)r;
    uint8_t scancode = inb(KBD_DATA);
    if (scancode & 0x80) return;                 // key release — ignore

    char c = scancode_ascii[scancode & 0x7F];
    if (!c) return;

    int next = (ktail + 1) % KBUF_SIZE;
    if (next != khead) { kbuf[ktail] = c; ktail = next; }   // drop if full
}

// Block until a character is available. Enables interrupts and halts while
// waiting (with the interrupt-disabled check guarding against a lost wakeup),
// so the keyboard IRQ can fill the buffer even when called from a syscall.
char keyboard_getchar(void) {
    for (;;) {
        __asm__ volatile ("cli");
        if (khead != ktail) {
            char c = kbuf[khead];
            khead = (khead + 1) % KBUF_SIZE;
            __asm__ volatile ("sti");
            return c;
        }
        __asm__ volatile ("sti; hlt");
    }
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_callback);   // IRQ1 -> vector 33
}
