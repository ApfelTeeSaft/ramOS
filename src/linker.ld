/* linker.ld - Linker script for ramOS kernel
 * 
 * This script tells the linker how to organize the kernel in memory.
 * The kernel is loaded at 1MB (0x00100000) which is standard for multiboot kernels.
 */

/* Entry point for the kernel */
ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 1MB physical address */
    . = 0x00100000;
    
    kernel_start = .;
    
    /* Multiboot header must be in first 8KB */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }
    
    /* Read-only code section */
    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data section */
    .rodata ALIGN(4K) : {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data section */
    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data section (BSS) */
    .bss ALIGN(4K) : {
        bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        bss_end = .;
    }
    
    kernel_end = .;
    
    /* Discard unneeded sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}