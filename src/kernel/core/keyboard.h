/* keyboard.h - PS/2 keyboard driver */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

/* Forward declaration - full definition in keyboard_loader.h */
typedef struct keyboard_layout_runtime keyboard_layout_runtime_t;

/* Initialize keyboard */
void keyboard_init(void);

/* Read line (blocking) */
void keyboard_read_line(char* buffer, size_t max_len);

/* Check if character is available */
int keyboard_has_char(void);

/* Get character (blocking) */
char keyboard_get_char(void);

/* Get keyboard state (shift, ctrl, alt, capslock) */
void keyboard_get_state(int* shift, int* ctrl, int* alt, int* caps);

/* Set runtime layout */
void keyboard_set_layout_runtime(keyboard_layout_runtime_t* layout);

#endif /* KEYBOARD_H */