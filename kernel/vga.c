// kernel/vga.c — VGA text-mode driver implementation.
#include "vga.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_COLS   80
#define VGA_ROWS   25

static size_t  cursor_row;
static size_t  cursor_col;
static uint8_t color;

// A VGA cell is: low byte = character, high byte = attribute (bg<<4 | fg).
static inline uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    color = (uint8_t)(fg | (bg << 4));
}

void vga_init(void) {
    cursor_row = 0;
    cursor_col = 0;
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    for (size_t y = 0; y < VGA_ROWS; y++)
        for (size_t x = 0; x < VGA_COLS; x++)
            VGA_MEMORY[y * VGA_COLS + x] = vga_entry(' ', color);
}

// Scroll everything up one line and clear the bottom row.
static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_ROWS; y++)
        for (size_t x = 0; x < VGA_COLS; x++)
            VGA_MEMORY[(y - 1) * VGA_COLS + x] = VGA_MEMORY[y * VGA_COLS + x];

    for (size_t x = 0; x < VGA_COLS; x++)
        VGA_MEMORY[(VGA_ROWS - 1) * VGA_COLS + x] = vga_entry(' ', color);

    cursor_row = VGA_ROWS - 1;
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else {
        VGA_MEMORY[cursor_row * VGA_COLS + cursor_col] = vga_entry(c, color);
        if (++cursor_col == VGA_COLS) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    if (cursor_row == VGA_ROWS)
        vga_scroll();
}

void vga_puts(const char *s) {
    for (size_t i = 0; s[i] != '\0'; i++)
        vga_putc(s[i]);
}

void vga_put_dec(uint32_t n) {
    if (n == 0) { vga_putc('0'); return; }
    char buf[10];
    int i = 0;
    while (n) { buf[i++] = (char)('0' + n % 10); n /= 10; }
    while (i) vga_putc(buf[--i]);
}

void vga_puts_at(size_t row, size_t col, const char *s) {
    size_t idx = row * VGA_COLS + col;
    for (size_t i = 0; s[i] != '\0' && idx < VGA_COLS * VGA_ROWS; i++, idx++)
        VGA_MEMORY[idx] = vga_entry(s[i], color);
}

uint8_t vga_get_color(void) { return color; }
void    vga_set_color_raw(uint8_t packed) { color = packed; }
