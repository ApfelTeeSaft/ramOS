/* kmain.c - Updated kernel entry point with all subsystems */

#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "../mm/memory.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../drivers/keyboard.h"
#include "../drivers/driver.h"
#include "../drivers/ata.h"
#include "../drivers/pci.h"
#include "../drivers/usb/usb_core.h"
#include "../drivers/usb/uhci.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../fs/ext4.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../proc/syscall.h"
#include "../shell.h"

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
    
    /* Print boot banner */
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    console_write("\n");
    console_write("========================================\n");
    console_write("  ramOS - Advanced Edition\n");
    console_write("  Booting...\n");
    console_write("========================================\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_write("\n");
    
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
    
    /* Initialize heap */
    console_write("[*] Initializing Heap...\n");
    heap_init();
    kprintf("    Heap initialized\n");
    
    /* Initialize VMM */
    console_write("[*] Initializing Virtual Memory...\n");
    vmm_init();
    
    /* Initialize keyboard */
    console_write("[*] Initializing Keyboard...\n");
    keyboard_init();
    
    /* Initialize PCI */
    console_write("[*] Initializing PCI...\n");
    pci_init();
    
    /* Initialize driver framework */
    console_write("[*] Initializing Driver Framework...\n");
    driver_init();
    
    /* Initialize ATA driver */
    console_write("[*] Initializing ATA Driver...\n");
    ata_init();
    
    /* Initialize USB */
    console_write("[*] Initializing USB...\n");
    uhci_init();
    
    /* Load initrd */
    console_write("[*] Loading initrd...\n");
    if (mboot->mods_count > 0) {
        struct multiboot_module* modules = (struct multiboot_module*)mboot->mods_addr;
        uint32_t initrd_addr = modules[0].mod_start;
        uint32_t initrd_size = modules[0].mod_end - modules[0].mod_start;
        
        kprintf("    initrd at 0x%x, size %u bytes\n", initrd_addr, initrd_size);
        
        int file_count = initrd_init(initrd_addr, initrd_size);
        kprintf("    Loaded %d files from initrd\n", file_count);
    } else {
        console_write("    Warning: No initrd found\n");
    }
    
    /* Initialize VFS */
    console_write("[*] Initializing VFS...\n");
    vfs_init();
    
    /* Initialize EXT4 */
    console_write("[*] Initializing EXT4...\n");
    ext4_init();
    
    /* Initialize process management */
    console_write("[*] Initializing Process Management...\n");
    process_init();
    
    /* Initialize scheduler */
    console_write("[*] Initializing Scheduler...\n");
    scheduler_init();
    
    /* Initialize system calls */
    console_write("[*] Initializing System Calls...\n");
    syscall_init();
    
    /* Boot complete */
    console_write("\n");
    console_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    console_write("[OK] Boot complete!\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_write("\n");
    
    /* Display system info */
    kprintf("ramOS v%s ready.\n", KERNEL_VERSION);
    kprintf("Type 'help' for available commands.\n");
    console_write("\n");
    
    /* Run shell */
    shell_run();
    
    /* Should never reach here */
    console_write("\nKernel exiting...\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}