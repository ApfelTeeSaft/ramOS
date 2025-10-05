/* keyboard_layout.h - Keyboard layout definitions */

#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#include <stdint.h>

/* Keyboard layout structure */
typedef struct {
    const char* name;
    const char* normal[128];    /* Normal key mappings */
    const char* shifted[128];   /* Shifted key mappings */
} keyboard_layout_t;

/* Available layouts */
extern const keyboard_layout_t layout_en_us;
extern const keyboard_layout_t layout_de_de;

/* Set active keyboard layout */
void keyboard_set_layout(const keyboard_layout_t* layout);

/* Get active keyboard layout */
const keyboard_layout_t* keyboard_get_layout(void);

/* Get character from scancode */
char keyboard_layout_get_char(uint8_t scancode, int shifted);

#endif /* KEYBOARD_LAYOUT_H */