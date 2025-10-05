/* keyboard_loader.h - Dynamic keyboard layout loader */

#ifndef KEYBOARD_LOADER_H
#define KEYBOARD_LOADER_H

#include <stdint.h>

/* Runtime keyboard layout structure */
typedef struct {
    char name[64];
    char code[32];
    char variant[32];
    char description[128];
    char normal[128];
    char shifted[128];
} keyboard_layout_runtime_t;

/* Initialize keyboard layout system */
void keyboard_layouts_init(void);

/* Load all available layouts from /etc/keyboard/layouts/ */
int keyboard_load_layouts(void);

/* Get layout by code (e.g., "en_US", "de_DE") */
keyboard_layout_runtime_t* keyboard_get_layout_by_code(const char* code);

/* List all available layouts */
void keyboard_list_layouts(void);

/* Set active layout by code */
int keyboard_set_active_layout(const char* code);

/* Get current layout code */
const char* keyboard_get_current_layout(void);

/* Load/save configuration */
int keyboard_load_config(void);
int keyboard_save_config(void);

#endif /* KEYBOARD_LOADER_H */