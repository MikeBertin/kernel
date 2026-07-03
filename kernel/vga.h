// kernel/vga.h — minimal VGA text-mode driver.
//
// In text mode the screen is memory-mapped: 80x25 cells starting at physical
// 0xB8000, two bytes per cell — an ASCII byte and an attribute byte (fg/bg
// colour). Writing there puts characters on screen directly. No BIOS, no
// driver stack; just memory.
#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

enum vga_color {
    VGA_BLACK = 0,  VGA_BLUE = 1,   VGA_GREEN = 2,   VGA_CYAN = 3,
    VGA_RED = 4,    VGA_MAGENTA = 5, VGA_BROWN = 6,  VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8, VGA_LIGHT_BLUE = 9, VGA_LIGHT_GREEN = 10, VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12, VGA_LIGHT_MAGENTA = 13, VGA_YELLOW = 14, VGA_WHITE = 15,
};

void vga_init(void);                          // clear screen, reset cursor/colour
void vga_set_color(uint8_t fg, uint8_t bg);   // colour for subsequent output
void vga_putc(char c);                        // one char (handles '\n' + scroll)
void vga_puts(const char *s);                 // NUL-terminated string
void vga_put_dec(uint32_t n);                 // unsigned decimal at the cursor

// Positioned write that does not disturb the streaming cursor — used for
// status readouts like the uptime clock.
void vga_puts_at(size_t row, size_t col, const char *s);

uint8_t vga_get_color(void);                  // current packed colour byte
void    vga_set_color_raw(uint8_t packed);    // restore a packed colour byte

#endif
