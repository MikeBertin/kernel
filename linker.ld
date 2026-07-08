/* linker.ld — lay out the kernel as a flat binary loaded at 0x10000.
 *
 * The boot sector loads the kernel to physical 0x10000 and jumps there, so
 * _start must land at exactly that address. We also carve the ring-3 shell's
 * code and read-only data into their own page-aligned section (.user_text) so
 * paging can mark just those pages user-accessible — everything else stays
 * supervisor-only and is genuinely off-limits to ring 3 (M5.1: protection). */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    /* Kernel code/data — everything EXCEPT the shell translation unit. */
    .text   : { *(EXCLUDE_FILE(*shell.o) .text   .text.*)   }
    .rodata : { *(EXCLUDE_FILE(*shell.o) .rodata .rodata.*) }
    .data   : { *(EXCLUDE_FILE(*shell.o) .data   .data.*)   }

    /* The ring-3 shell's code + literals, isolated on their own pages. */
    . = ALIGN(4096);
    .user_text : {
        user_text_start = .;
        *shell.o(.text .text.* .rodata .rodata.* .data .data.*)
        . = ALIGN(4096);
        user_text_end = .;
    }

    /* BSS (zeroed at startup). The shell's BSS — notably the page-aligned
       user_stack — lives here; paging maps just its pages user-accessible. */
    .bss : {
        bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        bss_end = .;
    }

    kernel_end = .;   /* frame allocator reserves everything below this */
}
