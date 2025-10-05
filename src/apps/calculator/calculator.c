/* calculator.c - Simple calculator application */

#include "../../api/libsys.h"

#define MAX_INPUT 256

/* Calculator state */
static double current_value = 0.0;
static char last_op = '=';
static int new_number = 1;

/* Simple double to string conversion */
static void dtoa(double value, char* buf, int precision) {
    int int_part = (int)value;
    itoa(int_part, buf, 10);
    
    int len = strlen(buf);
    buf[len++] = '.';
    
    double frac_part = value - int_part;
    if (frac_part < 0) frac_part = -frac_part;
    
    for (int i = 0; i < precision; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        buf[len++] = '0' + digit;
        frac_part -= digit;
    }
    
    buf[len] = '\0';
}

/* Simple string to double conversion */
static double atof(const char* str) {
    double result = 0.0;
    double sign = 1.0;
    int decimal_places = 0;
    int past_decimal = 0;
    
    /* Skip whitespace */
    while (*str == ' ') str++;
    
    /* Check sign */
    if (*str == '-') {
        sign = -1.0;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    /* Parse number */
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            if (past_decimal) {
                decimal_places++;
                result = result * 10.0 + (*str - '0');
            } else {
                result = result * 10.0 + (*str - '0');
            }
        } else if (*str == '.') {
            past_decimal = 1;
        } else {
            break;
        }
        str++;
    }
    
    /* Apply decimal places */
    while (decimal_places > 0) {
        result /= 10.0;
        decimal_places--;
    }
    
    return sign * result;
}

/* Perform calculation */
static void calculate(double input) {
    switch (last_op) {
        case '+':
            current_value += input;
            break;
        case '-':
            current_value -= input;
            break;
        case '*':
            current_value *= input;
            break;
        case '/':
            if (input != 0.0) {
                current_value /= input;
            } else {
                println("Error: Division by zero");
                return;
            }
            break;
        case '=':
        default:
            current_value = input;
            break;
    }
}

/* Display help */
static void show_help(void) {
    println("Calculator Commands:");
    println("  <number>     - Enter a number");
    println("  +, -, *, /   - Arithmetic operations");
    println("  =            - Calculate result");
    println("  c            - Clear");
    println("  h            - Show this help");
    println("  q            - Quit");
    println("");
}

/* Main calculator loop */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[MAX_INPUT];
    char display[64];
    
    println("========================================");
    println("  ramOS Calculator v1.0");
    println("========================================");
    println("");
    show_help();
    
    while (1) {
        /* Display current value */
        dtoa(current_value, display, 4);
        printf("Result: %s\n", display);
        print("> ");
        
        /* Read input */
        readln(input, MAX_INPUT);
        
        /* Skip empty input */
        if (input[0] == '\0') {
            continue;
        }
        
        /* Handle commands */
        if (input[0] == 'q') {
            println("Goodbye!");
            break;
        } else if (input[0] == 'h') {
            show_help();
            continue;
        } else if (input[0] == 'c') {
            current_value = 0.0;
            last_op = '=';
            new_number = 1;
            println("Cleared");
            continue;
        } else if (input[0] == '+' || input[0] == '-' || 
                   input[0] == '*' || input[0] == '/' || input[0] == '=') {
            if (!new_number) {
                last_op = input[0];
                new_number = 1;
            }
            continue;
        }
        
        /* Parse number */
        double value = atof(input);
        
        /* Perform calculation if not a new number */
        if (!new_number) {
            calculate(value);
        } else {
            current_value = value;
        }
        
        new_number = 0;
    }
    
    sys_exit(0);
    return 0;
}