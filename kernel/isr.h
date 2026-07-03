// kernel/isr.h — interrupt service routines and dispatch.
#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// The register snapshot the asm stubs build on the stack before calling C.
// The field order MUST match the push order in interrupts.asm exactly.
typedef struct registers {
    uint32_t ds;                                     // saved data segment
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushed by `pusha`
    uint32_t int_no, err_code;                       // pushed by our stub / CPU
    uint32_t eip, cs, eflags, useresp, ss;           // pushed by the CPU
} registers_t;

typedef void (*isr_t)(registers_t *);

void register_interrupt_handler(uint8_t n, isr_t handler);
void pic_remap(void);

// CPU exception stubs (0-31), defined in interrupts.asm.
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

// Hardware IRQ stubs (0-15), defined in interrupts.asm.
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

#endif
