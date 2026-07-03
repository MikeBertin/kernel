/* linker.ld — lay out the kernel as a flat binary loaded at 0x10000.
 *
 * The boot sector loads the kernel image to physical 0x10000 and jumps there,
 * so _start must land at exactly that address. We set the location counter to
 * 0x10000 and list entry.o first on the link line so its code comes first. */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        bss_start = .;
        *(COMMON)
        *(.bss*)
        bss_end = .;
    }

    kernel_end = .;   /* end of the kernel in memory — frame allocator reserves below this */
}
