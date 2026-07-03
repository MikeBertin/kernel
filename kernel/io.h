// kernel/io.h — x86 port I/O primitives.
//
// Devices like the PIC, PIT and keyboard controller live in a separate "I/O
// address space" reached with the in/out instructions rather than memory
// addresses. These inline wrappers are the whole interface to that world.
#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// A short, harmless write to an unused port — gives slow devices (the PIC) a
// moment to settle between commands.
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif
