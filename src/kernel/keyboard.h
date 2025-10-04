/* keyboard.h - PS/2 keyboard driver */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

/* Initialize keyboard */
void keyboard_init(void);

/* Read line (blocking) */
void keyboard_read_line(char* buffer, size_t max_len);

/* Check if character is available */
int keyboard_has_char(void);

/* Get character (non-blocking) */
char keyboard_get_char(void);

#endif /* KEYBOARD_H */