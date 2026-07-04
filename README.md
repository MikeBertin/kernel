# 🖥️ Project KERNEL — OS from Scratch

> *"An OS is just a program that never exits."*

## What This Is
Build a working operating system kernel from scratch. Not Linux from Scratch (which uses existing components) — writing the bootloader, memory manager, scheduler, and basic drivers by hand. Target: something that boots on real hardware (or QEMU), prints to screen, handles interrupts, and runs a simple userspace program.

## Goal & Why It Matters
Writing an OS removes the last abstraction. You understand what a process actually is, what memory management costs, why context switching is expensive, what a system call actually does. After BABEL (which teaches you what languages are) and KERNEL (which teaches you what they run on), you have no more black boxes.

This is the foundation beneath every piece of software ever written. Understanding it changes how you build everything above it.

## Milestones
| # | Milestone | Target |
|---|-----------|--------|
| 1 | Study: *Operating Systems: Three Easy Pieces* (Arpaci-Dusseau) + OSDev wiki | TBD |
| 2 | Bootloader: boot to protected mode, print "Hello" | TBD |
| 3 | Memory manager: paging, heap allocator | TBD |
| 4 | Interrupt handler + basic keyboard driver | TBD |
| 5 | Scheduler: preemptive multitasking, context switch | TBD |
| 6 | Basic shell running in userspace | TBD |

## Key Files
| File | Purpose |
|------|---------|
| `HANDOVER.md` | **Start here in a new session** — architecture, workflow, gotchas, next steps |
| `README.md` | This file |
| `STATUS.md` | Current state and next action |
| `log.md` | Decisions, progress, session notes |

## Notes
- Lives under FOUNDRY umbrella — sequence position: second (after BABEL, before PROMETHEUS)
- Implementation language: C + x86 assembly (traditional, best documented)
- Target: boots in QEMU first, real hardware stretch goal
- Reference: *Operating Systems: Three Easy Pieces* (free online), OSDev wiki (osdev.org), *Writing a Simple OS from Scratch* (Nick Blundell)
- SPECTRE (OSCP) will build low-level systems knowledge that feeds directly into this

> *Any quotes used in this project must be real, sourced attributions. No invented quotes.*
