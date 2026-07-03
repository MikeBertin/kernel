// kernel/keyboard.h — PS/2 keyboard driver.
//
// The keyboard controller raises IRQ1 whenever a key changes state and puts a
// "scancode" in port 0x60. We translate press scancodes to ASCII and echo them.
#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);   // install the IRQ1 handler
char keyboard_getchar(void);   // block until a key is available, return its ASCII

#endif
