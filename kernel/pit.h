// kernel/pit.h — the Programmable Interval Timer (Intel 8253/8254).
//
// The PIT fires IRQ0 at a fixed frequency, giving the kernel a steady heartbeat
// — the basis for uptime and, later, pre-emptive scheduling.
#ifndef PIT_H
#define PIT_H

#include <stdint.h>

void pit_init(uint32_t frequency_hz);  // start the timer, install IRQ0 handler
uint32_t pit_ticks(void);              // ticks since boot

// Register a function to run on every tick (used by the scheduler to pre-empt).
void pit_set_tick_hook(void (*fn)(void));

#endif
