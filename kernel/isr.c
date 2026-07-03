// kernel/isr.c — the C side of interrupt handling.
//
// The asm stubs all funnel into isr_handler (exceptions) or irq_handler
// (hardware interrupts), passing a pointer to the saved register frame.
#include "isr.h"
#include "io.h"
#include "vga.h"

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// --- PIC (8259) remap ---------------------------------------------------------
// The two cascaded PICs default to interrupt vectors 0x08-0x0F and 0x70-0x77.
// The low range collides with the CPU's own exception vectors (0-31), so a
// timer tick would look like a double fault. We reprogram them: master -> 0x20
// (32), slave -> 0x28 (40).
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20

void pic_remap(void) {
    outb(PIC1_CMD, 0x11); io_wait();   // ICW1: begin init, expect ICW4
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, 0x20); io_wait();  // ICW2: master vector offset = 32
    outb(PIC2_DATA, 0x28); io_wait();  // ICW2: slave vector offset  = 40
    outb(PIC1_DATA, 0x04); io_wait();  // ICW3: slave is on master IRQ2 (bitmask)
    outb(PIC2_DATA, 0x02); io_wait();  // ICW3: slave cascade identity
    outb(PIC1_DATA, 0x01); io_wait();  // ICW4: 8086 mode
    outb(PIC2_DATA, 0x01); io_wait();
    outb(PIC1_DATA, 0x00);             // clear all masks — enable every IRQ line
    outb(PIC2_DATA, 0x00);
}

// --- dispatch -----------------------------------------------------------------

// A CPU exception fired (divide-by-zero, GP fault, page fault, ...). With no
// registered handler we can't safely continue, so report and halt.
void isr_handler(registers_t *r) {
    isr_t h = interrupt_handlers[r->int_no];
    if (h) { h(r); return; }

    vga_set_color(VGA_WHITE, VGA_RED);
    vga_puts("\n[CPU EXCEPTION] vector ");
    vga_put_dec(r->int_no);
    vga_puts(" err=");
    vga_put_dec(r->err_code);
    vga_puts(" — system halted.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

// A hardware IRQ fired. We must acknowledge it (End Of Interrupt) or the PIC
// will never deliver another. Slave-line IRQs (>= 40) need an EOI to both PICs.
void irq_handler(registers_t *r) {
    if (r->int_no >= 40) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);

    isr_t h = interrupt_handlers[r->int_no];
    if (h) h(r);
}
