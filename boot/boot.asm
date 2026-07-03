; boot/boot.asm — Stage 1 boot sector (Milestone 1)
;
; The BIOS gives us 512 bytes and 16-bit real mode. That is not enough room
; for a real kernel, so stage 1's job grows: it must (1) load the C kernel off
; the disk into memory, (2) enable the A20 line, (3) install a GDT and switch
; the CPU into 32-bit protected mode, then (4) hand control to the kernel.
;
; After the far jump into protected mode there is no going back to BIOS
; services — from that point the machine is entirely ours.

[bits 16]
[org 0x7C00]

KERNEL_OFFSET   equ 0x10000     ; physical address we load the kernel to (64 KiB)
KERNEL_SECTORS  equ 32          ; how many 512-byte sectors of kernel to read

boot_start:
    mov [BOOT_DRIVE], dl        ; BIOS leaves the boot drive number in DL — save it

    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00             ; real-mode stack just below us
    sti

    ; Explicitly set VGA text mode 3 (80x25, 16 colour) via the video BIOS.
    ; We must do this in real mode, before the protected-mode switch, because
    ; it needs int 0x10. It guarantees the VGA registers are programmed so that
    ; 0xB8000 is the live text buffer — some BIOSes (e.g. v86's) don't do this
    ; for us, and our kernel writes straight to 0xB8000 assuming mode 3.
    mov ax, 0x0003
    int 0x10

    mov si, MSG_REAL
    call print_string

    call load_kernel          ; disk -> 0x10000
    call enable_a20
    call switch_to_pm         ; installs GDT, sets PE, far-jumps — never returns

    jmp $                     ; unreachable

; -----------------------------------------------------------------------------
; Real-mode helpers
; -----------------------------------------------------------------------------

; print_string — NUL-terminated string at DS:SI, via BIOS teletype
print_string:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; load_kernel — read KERNEL_SECTORS sectors starting at sector 2 into 0x10000.
; BIOS int 0x13, AH=0x02 reads CHS sectors into ES:BX.
load_kernel:
    mov si, MSG_LOAD
    call print_string

    mov bx, 0x1000            ; ES = 0x1000 -> segment base 0x10000
    mov es, bx
    xor bx, bx               ; ES:BX = 0x1000:0000 = physical 0x10000

    mov ah, 0x02             ; BIOS: read sectors
    mov al, KERNEL_SECTORS   ; sector count
    mov ch, 0x00             ; cylinder 0
    mov cl, 0x02             ; start at sector 2 (sector 1 = this boot sector)
    mov dh, 0x00             ; head 0
    mov dl, [BOOT_DRIVE]     ; drive we booted from
    int 0x13
    jc .error                ; carry set => read failed
    cmp al, KERNEL_SECTORS   ; AL = sectors actually read
    jne .error
    ret
.error:
    mov si, MSG_DISK_ERR
    call print_string
    jmp $

; enable_a20 — the "fast A20" gate via system control port 0x92. Without this,
; address line 20 is masked and memory wraps at 1 MiB (an 8086 compatibility
; wart we have to switch off before touching high memory).
enable_a20:
    in al, 0x92
    or al, 00000010b
    out 0x92, al
    ret

; switch_to_pm — load the GDT, set the protection-enable bit, far-jump to flush
; the prefetch pipeline and load a 32-bit code selector into CS.
switch_to_pm:
    cli                      ; no interrupts: we have no 32-bit IDT yet (that's M2)
    lgdt [gdt_descriptor]

    mov eax, cr0
    or  eax, 0x1             ; CR0.PE = 1
    mov cr0, eax

    jmp CODE_SEG:init_pm     ; far jump into 32-bit code

; -----------------------------------------------------------------------------
; 32-bit protected mode
; -----------------------------------------------------------------------------
[bits 32]
init_pm:
    mov ax, DATA_SEG         ; reload every segment register with the data selector
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000         ; a fresh 32-bit stack, comfortably above the kernel
    mov esp, ebp

    jmp KERNEL_OFFSET        ; into the kernel's 32-bit entry stub

; -----------------------------------------------------------------------------
; Global Descriptor Table — the classic "flat" model: two overlapping segments
; (code + data) each covering the whole 4 GiB address space. Segmentation
; effectively becomes a no-op so we can think in flat physical addresses.
; -----------------------------------------------------------------------------
gdt_start:
gdt_null:                    ; the mandatory null descriptor
    dd 0x0
    dd 0x0
gdt_code:                    ; base=0, limit=0xFFFFF, 4 KiB granularity => 4 GiB
    dw 0xFFFF                ; limit (bits 0-15)
    dw 0x0000                ; base  (bits 0-15)
    db 0x00                  ; base  (bits 16-23)
    db 10011010b             ; access: present, ring0, code, executable, readable
    db 11001111b             ; flags: gran=1, 32-bit=1 | limit (bits 16-19)
    db 0x00                  ; base  (bits 24-31)
gdt_data:                    ; same span, data segment (writable)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b             ; access: present, ring0, data, writable
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; size of the table, minus one
    dd gdt_start                 ; linear address of the table

; selectors = byte offset of the descriptor within the GDT
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; -----------------------------------------------------------------------------
; Data
; -----------------------------------------------------------------------------
BOOT_DRIVE    db 0
MSG_REAL      db "KERNEL: real mode.", 0x0D, 0x0A, 0
MSG_LOAD      db "KERNEL: loading kernel from disk...", 0x0D, 0x0A, 0
MSG_DISK_ERR  db "KERNEL: disk read error!", 0x0D, 0x0A, 0

; --- boot sector padding & signature -----------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55
