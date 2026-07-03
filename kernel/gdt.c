// kernel/gdt.c — GDT with kernel + user segments and a TSS.
#include "gdt.h"
#include "string.h"
#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;   // high limit nibble + flags
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// The 32-bit Task State Segment. We only really use ss0/esp0 — the ring-0 stack
// the CPU switches to on a trap from ring 3 — but the layout must be exact.
struct tss_entry {
    uint32_t prev;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gdtp;
static struct tss_entry tss;

// A dedicated ring-0 stack the CPU switches to whenever a trap arrives from
// ring 3 (syscalls, interrupts). Separate from the boot stack.
static uint8_t kernel_stack[16384];

extern void gdt_flush(uint32_t gdt_ptr);   // lgdt + reload segment registers
extern void tss_flush(void);               // ltr

static void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access      = access;
}

void gdt_install(void) {
    // access bytes: P|DPL|S|Ex|DC|RW|Ac.  gran 0xCF = 4 KiB pages, 32-bit.
    set_entry(0, 0, 0, 0, 0);                 // 0x00 null
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);     // 0x08 kernel code (ring 0)
    set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);     // 0x10 kernel data (ring 0)
    set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);     // 0x18 user code   (ring 3)
    set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);     // 0x20 user data   (ring 3)

    // 0x28 TSS: type 0x89 = present, available 32-bit TSS.
    memset(&tss, 0, sizeof(tss));
    tss.ss0  = 0x10;                                    // kernel data segment
    tss.esp0 = (uint32_t)(kernel_stack + sizeof(kernel_stack));
    tss.iomap_base = sizeof(tss);
    set_entry(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint32_t)&gdt;

    gdt_flush((uint32_t)&gdtp);
    tss_flush();
}
