/* keyboard_layout.c - Keyboard layout implementation (Legacy/Fallback) */

#include "keyboard_layout.h"

static const keyboard_layout_t* active_layout = NULL;

/* US QWERTY Layout - FALLBACK ONLY */
static const char* en_us_normal[128] = {
    [0x00] = NULL,  [0x01] = "\x1B", [0x02] = "1",    [0x03] = "2",
    [0x04] = "3",   [0x05] = "4",    [0x06] = "5",    [0x07] = "6",
    [0x08] = "7",   [0x09] = "8",    [0x0A] = "9",    [0x0B] = "0",
    [0x0C] = "-",   [0x0D] = "=",    [0x0E] = "\b",   [0x0F] = "\t",
    [0x10] = "q",   [0x11] = "w",    [0x12] = "e",    [0x13] = "r",
    [0x14] = "t",   [0x15] = "y",    [0x16] = "u",    [0x17] = "i",
    [0x18] = "o",   [0x19] = "p",    [0x1A] = "[",    [0x1B] = "]",
    [0x1C] = "\n",  [0x1D] = NULL,   [0x1E] = "a",    [0x1F] = "s",
    [0x20] = "d",   [0x21] = "f",    [0x22] = "g",    [0x23] = "h",
    [0x24] = "j",   [0x25] = "k",    [0x26] = "l",    [0x27] = ";",
    [0x28] = "'",   [0x29] = "`",    [0x2A] = NULL,   [0x2B] = "\\",
    [0x2C] = "z",   [0x2D] = "x",    [0x2E] = "c",    [0x2F] = "v",
    [0x30] = "b",   [0x31] = "n",    [0x32] = "m",    [0x33] = ",",
    [0x34] = ".",   [0x35] = "/",    [0x36] = NULL,   [0x37] = "*",
    [0x38] = NULL,  [0x39] = " ",    [0x3A] = NULL,   [0x3B ... 0x7F] = NULL
};

static const char* en_us_shifted[128] = {
    [0x00] = NULL,  [0x01] = "\x1B", [0x02] = "!",    [0x03] = "@",
    [0x04] = "#",   [0x05] = "$",    [0x06] = "%",    [0x07] = "^",
    [0x08] = "&",   [0x09] = "*",    [0x0A] = "(",    [0x0B] = ")",
    [0x0C] = "_",   [0x0D] = "+",    [0x0E] = "\b",   [0x0F] = "\t",
    [0x10] = "Q",   [0x11] = "W",    [0x12] = "E",    [0x13] = "R",
    [0x14] = "T",   [0x15] = "Y",    [0x16] = "U",    [0x17] = "I",
    [0x18] = "O",   [0x19] = "P",    [0x1A] = "{",    [0x1B] = "}",
    [0x1C] = "\n",  [0x1D] = NULL,   [0x1E] = "A",    [0x1F] = "S",
    [0x20] = "D",   [0x21] = "F",    [0x22] = "G",    [0x23] = "H",
    [0x24] = "J",   [0x25] = "K",    [0x26] = "L",    [0x27] = ":",
    [0x28] = "\"",  [0x29] = "~",    [0x2A] = NULL,   [0x2B] = "|",
    [0x2C] = "Z",   [0x2D] = "X",    [0x2E] = "C",    [0x2F] = "V",
    [0x30] = "B",   [0x31] = "N",    [0x32] = "M",    [0x33] = "<",
    [0x34] = ">",   [0x35] = "?",    [0x36] = NULL,   [0x37] = "*",
    [0x38] = NULL,  [0x39] = " ",    [0x3A] = NULL,   [0x3B ... 0x7F] = NULL
};

const keyboard_layout_t layout_en_us = {
    .name = "en_US",
    .normal = en_us_normal,
    .shifted = en_us_shifted
};

/* German QWERTZ Layout - FALLBACK ONLY */
static const char* de_de_normal[128] = {
    [0x00] = NULL,  [0x01] = "\x1B", [0x02] = "1",    [0x03] = "2",
    [0x04] = "3",   [0x05] = "4",    [0x06] = "5",    [0x07] = "6",
    [0x08] = "7",   [0x09] = "8",    [0x0A] = "9",    [0x0B] = "0",
    [0x0C] = "\xDF",[0x0D] = "\xB4", [0x0E] = "\b",   [0x0F] = "\t",
    [0x10] = "q",   [0x11] = "w",    [0x12] = "e",    [0x13] = "r",
    [0x14] = "t",   [0x15] = "z",    [0x16] = "u",    [0x17] = "i",
    [0x18] = "o",   [0x19] = "p",    [0x1A] = "\xFC", [0x1B] = "+",
    [0x1C] = "\n",  [0x1D] = NULL,   [0x1E] = "a",    [0x1F] = "s",
    [0x20] = "d",   [0x21] = "f",    [0x22] = "g",    [0x23] = "h",
    [0x24] = "j",   [0x25] = "k",    [0x26] = "l",    [0x27] = "\xF6",
    [0x28] = "\xE4",[0x29] = "^",    [0x2A] = NULL,   [0x2B] = "#",
    [0x2C] = "y",   [0x2D] = "x",    [0x2E] = "c",    [0x2F] = "v",
    [0x30] = "b",   [0x31] = "n",    [0x32] = "m",    [0x33] = ",",
    [0x34] = ".",   [0x35] = "-",    [0x36] = NULL,   [0x37] = "*",
    [0x38] = NULL,  [0x39] = " ",    [0x3A] = NULL,   [0x3B ... 0x7F] = NULL
};

static const char* de_de_shifted[128] = {
    [0x00] = NULL,  [0x01] = "\x1B", [0x02] = "!",    [0x03] = "\"",
    [0x04] = "\xA7",[0x05] = "$",    [0x06] = "%",    [0x07] = "&",
    [0x08] = "/",   [0x09] = "(",    [0x0A] = ")",    [0x0B] = "=",
    [0x0C] = "?",   [0x0D] = "`",    [0x0E] = "\b",   [0x0F] = "\t",
    [0x10] = "Q",   [0x11] = "W",    [0x12] = "E",    [0x13] = "R",
    [0x14] = "T",   [0x15] = "Z",    [0x16] = "U",    [0x17] = "I",
    [0x18] = "O",   [0x19] = "P",    [0x1A] = "\xDC", [0x1B] = "*",
    [0x1C] = "\n",  [0x1D] = NULL,   [0x1E] = "A",    [0x1F] = "S",
    [0x20] = "D",   [0x21] = "F",    [0x22] = "G",    [0x23] = "H",
    [0x24] = "J",   [0x25] = "K",    [0x26] = "L",    [0x27] = "\xD6",
    [0x28] = "\xC4",[0x29] = "\xB0", [0x2A] = NULL,   [0x2B] = "'",
    [0x2C] = "Y",   [0x2D] = "X",    [0x2E] = "C",    [0x2F] = "V",
    [0x30] = "B",   [0x31] = "N",    [0x32] = "M",    [0x33] = ";",
    [0x34] = ":",   [0x35] = "_",    [0x36] = NULL,   [0x37] = "*",
    [0x38] = NULL,  [0x39] = " ",    [0x3A] = NULL,   [0x3B ... 0x7F] = NULL
};

const keyboard_layout_t layout_de_de = {
    .name = "de_DE",
    .normal = de_de_normal,
    .shifted = de_de_shifted
};

/* Set active keyboard layout - LEGACY FUNCTION */
void keyboard_set_layout(const keyboard_layout_t* layout) {
    if (layout) {
        active_layout = layout;
    }
}

/* Get active keyboard layout - LEGACY FUNCTION */
const keyboard_layout_t* keyboard_get_layout(void) {
    if (!active_layout) {
        active_layout = &layout_en_us;  /* Default to US layout */
    }
    return active_layout;
}

/* Get character from scancode - FALLBACK FUNCTION */
char keyboard_layout_get_char(uint8_t scancode, int shifted) {
    if (scancode >= 128) return 0;
    
    const keyboard_layout_t* layout = keyboard_get_layout();
    const char* mapping = shifted ? layout->shifted[scancode] : layout->normal[scancode];
    
    if (!mapping) return 0;
    return mapping[0];
}