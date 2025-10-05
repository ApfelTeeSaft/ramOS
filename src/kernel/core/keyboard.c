/* keyboard.c - PS/2 keyboard driver with dynamic layout support */

#include "keyboard.h"
#include "keyboard_layout.h"
#include "keyboard_loader.h"
#include "irq.h"
#include "isr.h"
#include "console.h"

/* Keyboard I/O port */
#define KEYBOARD_DATA_PORT 0x60

/* Active runtime layout */
static keyboard_layout_runtime_t* active_runtime_layout = NULL;

/* Special scancodes */
#define KEY_LSHIFT_PRESS   0x2A
#define KEY_RSHIFT_PRESS   0x36
#define KEY_LSHIFT_RELEASE 0xAA
#define KEY_RSHIFT_RELEASE 0xB6
#define KEY_LCTRL_PRESS    0x1D
#define KEY_LCTRL_RELEASE  0x9D
#define KEY_LALT_PRESS     0x38
#define KEY_LALT_RELEASE   0xB8
#define KEY_CAPSLOCK       0x3A

/* Keyboard buffer */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_read_pos = 0;
static volatile int kb_write_pos = 0;

/* Keyboard state */
static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;
static volatile int alt_pressed = 0;
static volatile int capslock_active = 0;

/* Port I/O */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Keyboard IRQ handler */
static void keyboard_handler(registers_t* regs) {
    (void)regs;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Handle special keys */
    switch (scancode) {
        case KEY_LSHIFT_PRESS:
        case KEY_RSHIFT_PRESS:
            shift_pressed = 1;
            return;
        
        case KEY_LSHIFT_RELEASE:
        case KEY_RSHIFT_RELEASE:
            shift_pressed = 0;
            return;
        
        case KEY_LCTRL_PRESS:
            ctrl_pressed = 1;
            return;
        
        case KEY_LCTRL_RELEASE:
            ctrl_pressed = 0;
            return;
        
        case KEY_LALT_PRESS:
            alt_pressed = 1;
            return;
        
        case KEY_LALT_RELEASE:
            alt_pressed = 0;
            return;
        
        case KEY_CAPSLOCK:
            capslock_active = !capslock_active;
            return;
    }
    
    /* Ignore key release events (bit 7 set) */
    if (scancode & 0x80) {
        return;
    }
    
    /* Get character from layout */
    char c = 0;
    if (active_runtime_layout && scancode < 128) {
        /* Use runtime layout if available */
        c = shift_pressed ? active_runtime_layout->shifted[scancode] : 
                           active_runtime_layout->normal[scancode];
    } else {
        /* Fallback to static layout */
        c = keyboard_layout_get_char(scancode, shift_pressed);
    }
    
    /* Handle capslock for letters */
    if (capslock_active && !shift_pressed && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (capslock_active && shift_pressed && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    if (c != 0) {
        /* Add to buffer */
        int next_pos = (kb_write_pos + 1) % KB_BUFFER_SIZE;
        if (next_pos != kb_read_pos) {
            kb_buffer[kb_write_pos] = c;
            kb_write_pos = next_pos;
        }
    }
}

void keyboard_init(void) {
    /* Register keyboard IRQ handler (IRQ 1) */
    irq_register_handler(1, keyboard_handler);
    
    /* Set default layout to US (will be overridden by loader) */
    keyboard_set_layout(&layout_en_us);
}

/* Set runtime layout */
void keyboard_set_layout_runtime(keyboard_layout_runtime_t* layout) {
    active_runtime_layout = layout;
    if (layout) {
        extern void kprintf(const char* fmt, ...);
        kprintf("[KEYBOARD] Layout changed to: %s\n", layout->name);
    }
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
        } else if ((unsigned char)c >= 128) {
            /* Extended ASCII character (for German umlauts, etc) */
            buffer[pos++] = c;
            console_putchar(c);
        }
    }
    
    buffer[pos] = '\0';
}

/* Get keyboard state for debugging */
void keyboard_get_state(int* shift, int* ctrl, int* alt, int* caps) {
    if (shift) *shift = shift_pressed;
    if (ctrl) *ctrl = ctrl_pressed;
    if (alt) *alt = alt_pressed;
    if (caps) *caps = capslock_active;
}