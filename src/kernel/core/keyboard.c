/* keyboard.c - PS/2 keyboard driver implementation */

#include "keyboard.h"
#include "irq.h"
#include "isr.h"
#include "console.h"

/* Keyboard I/O port */
#define KEYBOARD_DATA_PORT 0x60

/* Keyboard buffer */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_read_pos = 0;
static volatile int kb_write_pos = 0;

/* Port I/O */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* US keyboard layout (scancode set 1) - lowercase only for simplicity */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Left Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, /* Right Shift */
    '*',
    0, /* Left Alt */
    ' ', /* Space */
};

/* Keyboard IRQ handler */
static void keyboard_handler(registers_t* regs) {
    (void)regs;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Ignore key release events (bit 7 set) */
    if (scancode & 0x80) {
        return;
    }
    
    /* Convert scancode to ASCII */
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            /* Add to buffer */
            int next_pos = (kb_write_pos + 1) % KB_BUFFER_SIZE;
            if (next_pos != kb_read_pos) {
                kb_buffer[kb_write_pos] = c;
                kb_write_pos = next_pos;
            }
        }
    }
}

void keyboard_init(void) {
    /* Register keyboard IRQ handler (IRQ 1) */
    irq_register_handler(1, keyboard_handler);
}

int keyboard_has_char(void) {
    return kb_read_pos != kb_write_pos;
}

char keyboard_get_char(void) {
    while (!keyboard_has_char()) {
        /* Wait for character */
        __asm__ volatile("hlt");
    }
    
    char c = kb_buffer[kb_read_pos];
    kb_read_pos = (kb_read_pos + 1) % KB_BUFFER_SIZE;
    return c;
}

void keyboard_read_line(char* buffer, size_t max_len) {
    size_t pos = 0;
    
    while (pos < max_len - 1) {
        char c = keyboard_get_char();
        
        if (c == '\n') {
            console_putchar('\n');
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                console_write("\b \b");
            }
        } else if (c >= 32 && c <= 126) {
            buffer[pos++] = c;
            console_putchar(c);
        }
    }
    
    buffer[pos] = '\0';
}