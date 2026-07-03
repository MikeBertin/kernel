; kernel/interrupts.asm — the low-level interrupt entry points.
;
; The CPU can't call a C function directly on an interrupt: it just pushes a
; minimal frame and jumps to the address in the IDT. These stubs finish building
; a uniform register frame, switch to kernel data segments, and call into C.
;
; Some CPU exceptions push an error code, others don't. To give every handler an
; identical stack layout we push a dummy 0 for the ones that don't.

[bits 32]

extern isr_handler
extern irq_handler

; --- exception stubs (vectors 0-31) ------------------------------------------
%macro ISR_NOERR 1
  global isr%1
  isr%1:
    push dword 0        ; dummy error code
    push dword %1       ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
  global isr%1
  isr%1:
    push dword %1       ; interrupt number (CPU already pushed the error code)
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; --- hardware IRQ stubs (vectors 32-47) --------------------------------------
%macro IRQ 2
  global irq%1
  irq%1:
    push dword 0        ; no error code for hardware IRQs
    push dword %2       ; remapped interrupt number
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; --- the syscall vector (int 0x80) — reuses the exception common path ---
ISR_NOERR 128

; --- common tails ------------------------------------------------------------
; Save the rest of the CPU state, load kernel data segments, hand a pointer to
; the register frame to C, then unwind and iret.

isr_common_stub:
    pusha                   ; edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov ax, ds
    push eax                ; save data segment selector
    mov ax, 0x10            ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; arg: registers_t*
    call isr_handler
    add esp, 4

    pop eax                 ; restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8              ; discard int_no + err_code
    iret                    ; restores eip, cs, eflags (re-enables interrupts)

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; --- load the IDT ------------------------------------------------------------
global idt_flush
idt_flush:
    mov eax, [esp + 4]      ; pointer to the idt_ptr
    lidt [eax]
    ret
