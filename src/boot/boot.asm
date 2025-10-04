; boot.asm - Multiboot v1 compliant bootloader for ramOS
; This file provides the entry point and prepares the system for the C kernel
;
; Boot sequence:
; 1. GRUB loads this code at the multiboot header
; 2. We verify multiboot bootloader loaded us correctly
; 3. Set up stack
; 4. Jump to C kernel entry (kmain)

bits 32                         ; We're in 32-bit protected mode (GRUB does this for us)

; Multiboot header constants (Multiboot v1 specification)
MULTIBOOT_MAGIC         equ 0x1BADB002      ; Magic number for multiboot
MULTIBOOT_ALIGN         equ 1<<0            ; Align loaded modules on page boundaries
MULTIBOOT_MEMINFO       equ 1<<1            ; Request memory map
MULTIBOOT_FLAGS         equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Reserve space for stack
STACK_SIZE              equ 0x4000          ; 16KB stack

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb STACK_SIZE                          ; Reserve 16KB for stack
stack_top:

section .text
global _start
extern kmain                                 ; C kernel entry point

_start:
    ; At this point:
    ; - We're in 32-bit protected mode (GRUB set this up)
    ; - Interrupts are disabled
    ; - Paging is disabled
    ; - EAX contains multiboot magic (0x2BADB002)
    ; - EBX contains physical address of multiboot info structure
    
    ; Set up stack pointer
    mov esp, stack_top
    mov ebp, esp
    
    ; Reset EFLAGS
    push 0
    popf
    
    ; Verify multiboot bootloader
    ; EAX should contain 0x2BADB002 if loaded by multiboot-compliant bootloader
    cmp eax, 0x2BADB002
    jne .no_multiboot
    
    ; Push multiboot info structure pointer (EBX) and magic (EAX) onto stack
    ; These will be parameters to kmain(uint32_t magic, uint32_t* mboot_info)
    push ebx                                 ; Multiboot info structure
    push eax                                 ; Multiboot magic number
    
    ; Call the C kernel
    call kmain
    
    ; If kmain returns, halt the system
    cli                                      ; Disable interrupts
.hang:
    hlt                                      ; Halt CPU
    jmp .hang                                ; Infinite loop in case of NMI
    
.no_multiboot:
    ; No multiboot compliant bootloader detected
    ; Just hang since we can't do much without proper setup
    cli
    hlt
    jmp .no_multiboot