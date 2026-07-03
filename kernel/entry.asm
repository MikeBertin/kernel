; kernel/entry.asm — 32-bit kernel entry stub
;
; The boot sector far-jumps to physical 0x10000 with a flat GDT already loaded
; and a stack set up. Thanks to the linker script, _start sits at exactly
; 0x10000, so this is the very first kernel code to run. Its only job is to
; call into C and, if kmain ever returns, halt forever.

[bits 32]
[extern kmain]                 ; defined in kmain.c
global _start

_start:
    call kmain

.hang:
    cli
    hlt
    jmp .hang
