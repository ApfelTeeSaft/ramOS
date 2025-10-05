/* app.ld - Linker script for ramOS applications */

ENTRY(_start)

SECTIONS
{
    /* Applications loaded at 0x10000000 (256MB) */
    . = 0x10000000;
    
    .text : {
        *(.text.start)
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        bss_end = .;
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}