/* app_start.c - Application startup code
 * 
 * This file provides the entry point for userspace applications.
 * It sets up the environment and calls the application's main() function.
 */

#include "../../api/libsys.h"

/* External symbols from linker script */
extern char bss_start[];
extern char bss_end[];

/* Main function prototype */
extern int main(int argc, char* argv[]);

/* Application entry point */
void _start(int argc, char* argv[]) __attribute__((section(".text.start")));

void _start(int argc, char* argv[]) {
    /* Clear BSS section */
    char* bss = bss_start;
    while (bss < bss_end) {
        *bss++ = 0;
    }
    
    /* Call main */
    int exit_code = main(argc, argv);
    
    /* Exit */
    sys_exit(exit_code);
    
    /* Should never reach here */
    while (1);
}