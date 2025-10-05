/* console.c - VGA text mode console implementation */

#include "console.h"

/* VGA text mode buffer */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static size_t cursor_x = 0;
static size_t cursor_y = 0;
static uint8_t current_color = 0x07;  /* Light grey on black */

/* Helper: create VGA entry */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Helper: create color byte */
static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

/* Forward declarations for kprintf helpers */
static void print_string(const char* s);
static void print_int(int n);
static void print_uint(unsigned int n);
static void print_hex(unsigned int n);

/* Scroll screen up one line */
static void console_scroll(void) {
    /* Move all lines up */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear bottom line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }
    
    cursor_y = VGA_HEIGHT - 1;
}

void console_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_clear();
}

void console_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void console_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = vga_color(fg, bg);
}

void console_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, current_color);
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        console_scroll();
    }
}

void console_write(const char* str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_writen(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        console_putchar(str[i]);
    }
}

/* Helper functions for kprintf */
static void print_string(const char* s) {
    console_write(s);
}

static void print_int(int n) {
    if (n < 0) {
        console_putchar('-');
        n = -n;
    }
    
    char buf[12];
    int i = 0;
    
    if (n == 0) {
        console_putchar('0');
        return;
    }
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

static void print_uint(unsigned int n) {
    char buf[12];
    int i = 0;
    
    if (n == 0) {
        console_putchar('0');
        return;
    }
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

static void print_hex(unsigned int n) {
    const char* hex = "0123456789ABCDEF";
    char buf[9];
    int i = 0;
    
    if (n == 0) {
        console_write("0x0");
        return;
    }
    
    while (n > 0) {
        buf[i++] = hex[n & 0xF];
        n >>= 4;
    }
    
    console_write("0x");
    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

/* printf implementation */
void kprintf(const char* fmt, ...) {
    uint32_t* args = (uint32_t*)((char*)&fmt + sizeof(fmt));
    int arg_index = 0;
    
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's':
                    print_string((const char*)args[arg_index++]);
                    break;
                case 'd':
                    print_int((int)args[arg_index++]);
                    break;
                case 'u':
                    print_uint((unsigned int)args[arg_index++]);
                    break;
                case 'x':
                    print_hex((unsigned int)args[arg_index++]);
                    break;
                case 'c':
                    console_putchar((char)args[arg_index++]);
                    break;
                case '%':
                    console_putchar('%');
                    break;
                default:
                    console_putchar('%');
                    console_putchar(*fmt);
                    break;
            }
        } else {
            console_putchar(*fmt);
        }
        fmt++;
    }
}