// kernel/idt.h — the Interrupt Descriptor Table.
//
// The IDT is the CPU's jump table for interrupts: 256 entries, one per vector.
// When interrupt N fires, the CPU looks up entry N and transfers control to the
// handler address stored there. lidt tells the CPU where the table lives.
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// One 8-byte gate descriptor. The 32-bit handler address is split across two
// 16-bit halves for backwards-compatibility with the 286's format.
struct idt_entry {
    uint16_t base_low;   // handler address, bits 0-15
    uint16_t selector;   // code segment selector (our kernel code = 0x08)
    uint8_t  always0;    // reserved, always 0
    uint8_t  flags;      // present, ring, gate type (0x8E = present ring0 int gate)
    uint16_t base_high;  // handler address, bits 16-31
} __attribute__((packed));

// The operand to lidt: size and base address of the table.
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_install(void);   // build the table, remap the PIC, load it

#endif
