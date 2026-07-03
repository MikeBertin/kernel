; kernel/gdt.asm — load the GDT/TSS and drop to ring 3.

[bits 32]
global gdt_flush
global tss_flush
global enter_usermode

; gdt_flush(uint32_t gdt_ptr) — install the GDT and reload every segment.
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    mov ax, 0x10        ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.reload_cs  ; far jump reloads CS with the kernel code selector
.reload_cs:
    ret

; tss_flush() — load the task register with the TSS selector (0x28).
tss_flush:
    mov ax, 0x28
    ltr ax
    ret

; enter_usermode(uint32_t eip, uint32_t esp) — build an iret frame that returns
; into ring 3. iret is the only way to lower the privilege level.
enter_usermode:
    mov eax, [esp + 4]   ; user entry point
    mov ebx, [esp + 8]   ; user stack top

    mov cx, 0x23         ; user data selector (0x20) | RPL 3
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23            ; ss  = user data | RPL 3
    push ebx             ; esp = user stack
    pushfd
    pop ecx
    or ecx, 0x200        ; set IF so interrupts stay on in ring 3
    push ecx             ; eflags
    push 0x1B            ; cs  = user code (0x18) | RPL 3
    push eax             ; eip = user entry
    iretd                ; drop to ring 3
