/* editor.c - Simple text editor application */

#include "../../api/libsys.h"

#define MAX_LINES 1000
#define MAX_LINE_LEN 256
#define MAX_FILENAME 128

/* Editor state */
typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;
    int current_line;
    char filename[MAX_FILENAME];
    int modified;
} editor_state_t;

static editor_state_t editor;

/* Initialize editor */
static void editor_init(void) {
    memset(&editor, 0, sizeof(editor_state_t));
    editor.line_count = 1;
    editor.current_line = 0;
    strcpy(editor.filename, "untitled.txt");
}

/* Display editor header */
static void show_header(void) {
    println("========================================");
    printf("  ramOS Editor - %s%s\n", editor.filename, 
           editor.modified ? " [modified]" : "");
    println("========================================");
}

/* Display help */
static void show_help(void) {
    println("\nEditor Commands:");
    println("  :w [file]  - Write (save) file");
    println("  :q         - Quit (warns if modified)");
    println("  :wq        - Write and quit");
    println("  :q!        - Quit without saving");
    println("  :l         - List all lines");
    println("  :d [line]  - Delete line");
    println("  :i [line]  - Insert at line");
    println("  :h         - Show this help");
    println("  <text>     - Append line");
    println("");
}

/* List all lines */
static void list_lines(void) {
    println("\n--- File Contents ---");
    for (int i = 0; i < editor.line_count; i++) {
        printf("%4d: %s\n", i + 1, editor.lines[i]);
    }
    printf("--- %d lines total ---\n\n", editor.line_count);
}

/* Load file */
static int load_file(const char* filename) {
    int fd = sys_open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Could not open file: %s\n", filename);
        return -1;
    }
    
    editor.line_count = 0;
    char buffer[4096];
    int bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
    sys_close(fd);
    
    if (bytes_read < 0) {
        println("Error reading file");
        return -1;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Parse lines */
    int line_pos = 0;
    for (int i = 0; i < bytes_read && editor.line_count < MAX_LINES; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            editor.lines[editor.line_count][line_pos] = '\0';
            editor.line_count++;
            line_pos = 0;
        } else if (line_pos < MAX_LINE_LEN - 1) {
            editor.lines[editor.line_count][line_pos++] = buffer[i];
        }
    }
    
    /* Handle last line */
    if (line_pos > 0 && editor.line_count < MAX_LINES) {
        editor.lines[editor.line_count][line_pos] = '\0';
        editor.line_count++;
    }
    
    if (editor.line_count == 0) {
        editor.line_count = 1;
    }
    
    strcpy(editor.filename, filename);
    editor.modified = 0;
    
    printf("Loaded %d lines from %s\n", editor.line_count, filename);
    return 0;
}

/* Save file */
static int save_file(const char* filename) {
    int fd = sys_open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        printf("Could not open file for writing: %s\n", filename);
        return -1;
    }
    
    for (int i = 0; i < editor.line_count; i++) {
        sys_write(fd, editor.lines[i], strlen(editor.lines[i]));
        sys_write(fd, "\n", 1);
    }
    
    sys_close(fd);
    
    strcpy(editor.filename, filename);
    editor.modified = 0;
    
    printf("Wrote %d lines to %s\n", editor.line_count, filename);
    return 0;
}

/* Insert line */
static void insert_line(int at_line, const char* text) {
    if (editor.line_count >= MAX_LINES) {
        println("Error: Maximum line count reached");
        return;
    }
    
    if (at_line < 0 || at_line > editor.line_count) {
        at_line = editor.line_count;
    }
    
    /* Shift lines down */
    for (int i = editor.line_count; i > at_line; i--) {
        strcpy(editor.lines[i], editor.lines[i - 1]);
    }
    
    /* Insert new line */
    strncpy(editor.lines[at_line], text, MAX_LINE_LEN - 1);
    editor.lines[at_line][MAX_LINE_LEN - 1] = '\0';
    editor.line_count++;
    editor.modified = 1;
}

/* Delete line */
static void delete_line(int line_num) {
    if (line_num < 0 || line_num >= editor.line_count) {
        println("Error: Invalid line number");
        return;
    }
    
    /* Shift lines up */
    for (int i = line_num; i < editor.line_count - 1; i++) {
        strcpy(editor.lines[i], editor.lines[i + 1]);
    }
    
    editor.line_count--;
    if (editor.line_count == 0) {
        editor.line_count = 1;
        editor.lines[0][0] = '\0';
    }
    editor.modified = 1;
    
    printf("Deleted line %d\n", line_num + 1);
}

/* Process command */
static int process_command(const char* cmd) {
    /* Write command */
    if (cmd[0] == 'w') {
        const char* filename = editor.filename;
        
        /* Check for filename argument */
        if (cmd[1] == ' ' && cmd[2] != '\0') {
            filename = cmd + 2;
        }
        
        return save_file(filename);
    }
    /* Quit command */
    else if (cmd[0] == 'q') {
        if (cmd[1] == '!') {
            return 1; /* Force quit */
        } else if (editor.modified) {
            println("Warning: File modified. Use :q! to quit without saving or :wq to save and quit");
            return 0;
        }
        return 1; /* Quit */
    }
    /* Write and quit */
    else if (strncmp(cmd, "wq", 2) == 0) {
        if (save_file(editor.filename) == 0) {
            return 1; /* Quit */
        }
        return 0;
    }
    /* List lines */
    else if (cmd[0] == 'l') {
        list_lines();
    }
    /* Delete line */
    else if (cmd[0] == 'd') {
        int line_num = editor.current_line;
        if (cmd[1] == ' ') {
            line_num = atoi(cmd + 2) - 1;
        }
        delete_line(line_num);
    }
    /* Insert line */
    else if (cmd[0] == 'i') {
        int line_num = editor.current_line;
        if (cmd[1] == ' ') {
            line_num = atoi(cmd + 2) - 1;
        }
        println("Enter text (empty line to finish):");
        char text[MAX_LINE_LEN];
        while (1) {
            print("  ");
            readln(text, MAX_LINE_LEN);
            if (text[0] == '\0') break;
            insert_line(line_num++, text);
        }
    }
    /* Help */
    else if (cmd[0] == 'h') {
        show_help();
    }
    else {
        println("Unknown command. Type :h for help");
    }
    
    return 0;
}

/* Main editor loop */
int main(int argc, char* argv[]) {
    char input[MAX_LINE_LEN];
    
    editor_init();
    
    /* Load file if specified */
    if (argc > 1) {
        load_file(argv[1]);
    }
    
    show_header();
    show_help();
    
    while (1) {
        print("> ");
        readln(input, MAX_LINE_LEN);
        
        /* Skip empty input */
        if (input[0] == '\0') {
            continue;
        }
        
        /* Handle commands (start with :) */
        if (input[0] == ':') {
            if (process_command(input + 1)) {
                break; /* Quit */
            }
        }
        /* Append line */
        else {
            if (editor.line_count < MAX_LINES) {
                strncpy(editor.lines[editor.line_count], input, MAX_LINE_LEN - 1);
                editor.lines[editor.line_count][MAX_LINE_LEN - 1] = '\0';
                editor.line_count++;
                editor.modified = 1;
                printf("Line %d added\n", editor.line_count);
            } else {
                println("Error: Maximum line count reached");
            }
        }
    }
    
    println("Editor closed");
    sys_exit(0);
    return 0;
}