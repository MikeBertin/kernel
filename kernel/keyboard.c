// kernel/keyboard.c — translate IRQ1 scancodes to characters and echo them.
#include "keyboard.h"
#include "isr.h"
#include "io.h"
#include "vga.h"

#define KBD_DATA 0x60

// Scancode set 1, US QWERTY. Index = make code; value = ASCII (0 = ignore).
// Only the printable keys plus a few controls are mapped — enough to type.
static const char scancode_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    /* remainder (F-keys, keypad, etc.) unmapped */
};

static void keyboard_callback(registers_t *r) {
    (void)r;
    uint8_t scancode = inb(KBD_DATA);

    // Top bit set means "key released" — ignore; we echo on press only.
    if (scancode & 0x80) return;

    char c = scancode_ascii[scancode & 0x7F];
    if (c) vga_putc(c);
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_callback);   // IRQ1 -> vector 33
}
