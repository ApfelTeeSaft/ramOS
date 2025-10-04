/* kmain.c - Kernel entry point */

#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "memory.h"
#include "keyboard.h"
#include "initrd.h"
#include "shell.h"

/* Multiboot header structure */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

/* Multiboot module structure */
struct multiboot_module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} __attribute__((packed));

/* Kernel entry point */
void kmain(uint32_t magic, struct multiboot_info* mboot) {
    /* Initialize console */
    console_init();
    
    /* Verify multiboot magic */
    if (magic != 0x2BADB002) {
        console_write("Error: Not loaded by multiboot bootloader!\n");
        return;
    }
    
    /* Print boot message */
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    console_write("ramOS - Booting...\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Initialize GDT */
    console_write("[*] Initializing GDT...\n");
    gdt_init();
    
    /* Initialize IDT */
    console_write("[*] Initializing IDT...\n");
    idt_init();
    
    /* Initialize ISRs */
    console_write("[*] Initializing ISRs...\n");
    isr_init();
    
    /* Initialize IRQs */
    console_write("[*] Initializing IRQs...\n");
    irq_init();
    
    /* Initialize timer */
    console_write("[*] Initializing Timer (PIT)...\n");
    timer_init();
    
    /* Initialize memory */
    console_write("[*] Initializing Memory...\n");
    uint32_t total_kb = mboot->mem_lower + mboot->mem_upper;
    memory_init(total_kb);
    kprintf("    Memory: %u KB total\n", total_kb);
    
    /* Initialize keyboard */
    console_write("[*] Initializing Keyboard...\n");
    keyboard_init();
    
    /* Load initrd if present */
    console_write("[*] Loading initrd...\n");
    if (mboot->mods_count > 0) {
        struct multiboot_module* modules = (struct multiboot_module*)mboot->mods_addr;
        uint32_t initrd_addr = modules[0].mod_start;
        uint32_t initrd_size = modules[0].mod_end - modules[0].mod_start;
        
        kprintf("    initrd at %x, size %u bytes\n", initrd_addr, initrd_size);
        
        int file_count = initrd_init(initrd_addr, initrd_size);
        kprintf("    Loaded %d files from initrd\n", file_count);
    } else {
        console_write("    Warning: No initrd found\n");
    }
    
    /* Boot complete */
    console_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    console_write("\n[OK] Boot complete!\n\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Run shell */
    shell_run();
    
    /* Should never reach here */
    console_write("\nKernel exiting...\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}