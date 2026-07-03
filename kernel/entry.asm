; kernel/entry.asm — 32-bit kernel entry stub
;
; The boot sector far-jumps to physical 0x10000 with a flat GDT loaded and a
; stack ready. The linker puts _start at exactly 0x10000, so this runs first.
; It zeroes the BSS (our loader copies only the on-disk image, not the
; zero-initialised region that follows) and calls into C.

[bits 32]
[extern kmain]
[extern bss_start]
[extern bss_end]
global _start

_start:
    ; zero the BSS: the IDT and handler tables live here and must start clear
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call kmain

.hang:
    hlt              ; sleep until an interrupt, then loop (interrupts stay on)
    jmp .hang
