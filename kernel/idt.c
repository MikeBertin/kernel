// kernel/idt.c — build and load the IDT.
#include "idt.h"
#include "isr.h"

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void idt_flush(uint32_t);   // in interrupts.asm: executes lidt

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

// Table of the 32 CPU-exception stubs and 16 IRQ stubs (defined in asm).
// 0x08 = kernel code selector; 0x8E = present, ring 0, 32-bit interrupt gate.
#define KCODE 0x08
#define GATE  0x8E

void idt_install(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;

    // Remap the PIC so hardware IRQs land on vectors 32-47 (see isr.c).
    pic_remap();

    // CPU exceptions 0-31.
    idt_set_gate(0,  (uint32_t)isr0,  KCODE, GATE);
    idt_set_gate(1,  (uint32_t)isr1,  KCODE, GATE);
    idt_set_gate(2,  (uint32_t)isr2,  KCODE, GATE);
    idt_set_gate(3,  (uint32_t)isr3,  KCODE, GATE);
    idt_set_gate(4,  (uint32_t)isr4,  KCODE, GATE);
    idt_set_gate(5,  (uint32_t)isr5,  KCODE, GATE);
    idt_set_gate(6,  (uint32_t)isr6,  KCODE, GATE);
    idt_set_gate(7,  (uint32_t)isr7,  KCODE, GATE);
    idt_set_gate(8,  (uint32_t)isr8,  KCODE, GATE);
    idt_set_gate(9,  (uint32_t)isr9,  KCODE, GATE);
    idt_set_gate(10, (uint32_t)isr10, KCODE, GATE);
    idt_set_gate(11, (uint32_t)isr11, KCODE, GATE);
    idt_set_gate(12, (uint32_t)isr12, KCODE, GATE);
    idt_set_gate(13, (uint32_t)isr13, KCODE, GATE);
    idt_set_gate(14, (uint32_t)isr14, KCODE, GATE);
    idt_set_gate(15, (uint32_t)isr15, KCODE, GATE);
    idt_set_gate(16, (uint32_t)isr16, KCODE, GATE);
    idt_set_gate(17, (uint32_t)isr17, KCODE, GATE);
    idt_set_gate(18, (uint32_t)isr18, KCODE, GATE);
    idt_set_gate(19, (uint32_t)isr19, KCODE, GATE);
    idt_set_gate(20, (uint32_t)isr20, KCODE, GATE);
    idt_set_gate(21, (uint32_t)isr21, KCODE, GATE);
    idt_set_gate(22, (uint32_t)isr22, KCODE, GATE);
    idt_set_gate(23, (uint32_t)isr23, KCODE, GATE);
    idt_set_gate(24, (uint32_t)isr24, KCODE, GATE);
    idt_set_gate(25, (uint32_t)isr25, KCODE, GATE);
    idt_set_gate(26, (uint32_t)isr26, KCODE, GATE);
    idt_set_gate(27, (uint32_t)isr27, KCODE, GATE);
    idt_set_gate(28, (uint32_t)isr28, KCODE, GATE);
    idt_set_gate(29, (uint32_t)isr29, KCODE, GATE);
    idt_set_gate(30, (uint32_t)isr30, KCODE, GATE);
    idt_set_gate(31, (uint32_t)isr31, KCODE, GATE);

    // Hardware IRQs 0-15, now at vectors 32-47.
    idt_set_gate(32, (uint32_t)irq0,  KCODE, GATE);
    idt_set_gate(33, (uint32_t)irq1,  KCODE, GATE);
    idt_set_gate(34, (uint32_t)irq2,  KCODE, GATE);
    idt_set_gate(35, (uint32_t)irq3,  KCODE, GATE);
    idt_set_gate(36, (uint32_t)irq4,  KCODE, GATE);
    idt_set_gate(37, (uint32_t)irq5,  KCODE, GATE);
    idt_set_gate(38, (uint32_t)irq6,  KCODE, GATE);
    idt_set_gate(39, (uint32_t)irq7,  KCODE, GATE);
    idt_set_gate(40, (uint32_t)irq8,  KCODE, GATE);
    idt_set_gate(41, (uint32_t)irq9,  KCODE, GATE);
    idt_set_gate(42, (uint32_t)irq10, KCODE, GATE);
    idt_set_gate(43, (uint32_t)irq11, KCODE, GATE);
    idt_set_gate(44, (uint32_t)irq12, KCODE, GATE);
    idt_set_gate(45, (uint32_t)irq13, KCODE, GATE);
    idt_set_gate(46, (uint32_t)irq14, KCODE, GATE);
    idt_set_gate(47, (uint32_t)irq15, KCODE, GATE);

    // int 0x80 syscall gate. 0xEE = present, DPL 3 (callable from ring 3),
    // 32-bit interrupt gate — the deliberate doorway for userspace.
    idt_set_gate(0x80, (uint32_t)isr128, KCODE, 0xEE);

    idt_flush((uint32_t)&idtp);
}
