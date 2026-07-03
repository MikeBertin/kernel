; kernel/switch.asm — the actual context switch.
;
; A "context switch" is startlingly small: save the callee-saved registers of
; the outgoing task, stash its stack pointer, load the incoming task's stack
; pointer, and restore its registers. Because the return address is also on the
; stack, the final `ret` resumes wherever the incoming task last left off.

[bits 32]
global context_switch
global task_trampoline

; void context_switch(uint32_t *save_old_esp, uint32_t new_esp)
context_switch:
    mov eax, [esp + 4]     ; save_old_esp — where to store the outgoing esp
    mov edx, [esp + 8]     ; new_esp      — the incoming task's saved esp

    push ebp               ; save the callee-saved registers the C ABI expects
    push ebx               ; us to preserve across a call
    push esi
    push edi

    mov [eax], esp         ; *save_old_esp = current esp  (freeze outgoing task)
    mov esp, edx           ; esp = new_esp                (adopt incoming task)

    pop edi                ; restore the incoming task's callee-saved registers
    pop esi
    pop ebx
    pop ebp
    ret                    ; resume the incoming task at its saved return address

; task_trampoline — where a brand-new task begins. A fresh task has never been
; interrupted, so we enable interrupts here (otherwise pre-emption would stop
; the instant we switched to it) and call its entry function, held in EBX by the
; initial stack we crafted in task_create.
task_trampoline:
    sti
    call ebx
.done:                     ; if the task function ever returns, halt safely
    cli
    hlt
    jmp .done
