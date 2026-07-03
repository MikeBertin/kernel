// kernel/pit.c — timer driver.
#include "pit.h"
#include "isr.h"
#include "io.h"
#include "vga.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_HZ  1193182u   // the PIT's fixed input clock

static volatile uint32_t ticks;
static uint32_t frequency;

uint32_t pit_ticks(void) { return ticks; }

// Tiny unsigned-to-decimal — no libc here.
static void utoa(uint32_t v, char *buf) {
    char tmp[11];
    int i = 0;
    if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (v) { tmp[i++] = (char)('0' + v % 10); v /= 10; }
    int j = 0;
    while (i) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

// IRQ0 handler: count ticks, and once per second refresh an uptime readout in
// the top-right corner so the timer is visibly alive without flooding the screen.
static void timer_callback(registers_t *r) {
    (void)r;
    ticks++;
    if (frequency && ticks % frequency == 0) {
        char line[24] = "uptime: ";
        char num[11];
        utoa(ticks / frequency, num);
        int p = 8;
        for (int k = 0; num[k]; k++) line[p++] = num[k];
        line[p++] = 's';
        while (p < 16) line[p++] = ' ';   // pad so digits shrinking still clears
        line[p] = '\0';
        uint8_t saved = vga_get_color();
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        vga_puts_at(0, 62, line);
        vga_set_color_raw(saved);
    }
}

void pit_init(uint32_t frequency_hz) {
    frequency = frequency_hz;
    register_interrupt_handler(32, timer_callback);   // IRQ0 -> vector 32

    uint32_t divisor = PIT_BASE_HZ / frequency_hz;
    outb(PIT_COMMAND, 0x36);                    // channel 0, lo/hi byte, mode 3
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
