// kernel/vga.c — VGA text-mode driver implementation.
#include "vga.h"
#include "io.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_COLS   80
#define VGA_ROWS   25

static size_t  cursor_row;
static size_t  cursor_col;
static uint8_t color;

// The streaming cursor writes and scrolls inside a window of rows, so a shell
// can scroll at the bottom of the screen while status lines above stay put.
static size_t  win_top    = 0;
static size_t  win_bottom = VGA_ROWS - 1;

// Move the blinking hardware cursor (CRT controller registers 0x0E/0x0F).
static void vga_update_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_row * VGA_COLS + cursor_col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));
}

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
    win_top = 0;
    win_bottom = VGA_ROWS - 1;
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    for (size_t y = 0; y < VGA_ROWS; y++)
        for (size_t x = 0; x < VGA_COLS; x++)
            VGA_MEMORY[y * VGA_COLS + x] = vga_entry(' ', color);
    vga_update_cursor();
}

// Confine streaming output to rows [top, bottom] and park the cursor at the top.
void vga_set_window(size_t top, size_t bottom) {
    win_top = top;
    win_bottom = bottom;
    cursor_row = top;
    cursor_col = 0;
    vga_update_cursor();
}

// Clear just the window (the shell's `clear`), leaving the rest of the screen.
void vga_clear_window(void) {
    for (size_t y = win_top; y <= win_bottom; y++)
        for (size_t x = 0; x < VGA_COLS; x++)
            VGA_MEMORY[y * VGA_COLS + x] = vga_entry(' ', color);
    cursor_row = win_top;
    cursor_col = 0;
    vga_update_cursor();
}

// Scroll the window up one line and clear its bottom row.
static void vga_scroll(void) {
    for (size_t y = win_top + 1; y <= win_bottom; y++)
        for (size_t x = 0; x < VGA_COLS; x++)
            VGA_MEMORY[(y - 1) * VGA_COLS + x] = VGA_MEMORY[y * VGA_COLS + x];

    for (size_t x = 0; x < VGA_COLS; x++)
        VGA_MEMORY[win_bottom * VGA_COLS + x] = vga_entry(' ', color);

    cursor_row = win_bottom;
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\b') {
        if (cursor_col > 0) cursor_col--;
    } else {
        VGA_MEMORY[cursor_row * VGA_COLS + cursor_col] = vga_entry(c, color);
        if (++cursor_col == VGA_COLS) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    if (cursor_row > win_bottom)
        vga_scroll();
    vga_update_cursor();
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

void vga_put_hex(uint32_t n) {
    static const char digits[] = "0123456789ABCDEF";
    vga_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4)
        vga_putc(digits[(n >> shift) & 0xF]);
}

void vga_puts_at(size_t row, size_t col, const char *s) {
    size_t idx = row * VGA_COLS + col;
    for (size_t i = 0; s[i] != '\0' && idx < VGA_COLS * VGA_ROWS; i++, idx++)
        VGA_MEMORY[idx] = vga_entry(s[i], color);
}

uint8_t vga_get_color(void) { return color; }
void    vga_set_color_raw(uint8_t packed) { color = packed; }

void vga_set_cursor(size_t row, size_t col) {
    cursor_row = row;
    cursor_col = col;
    vga_update_cursor();
}
