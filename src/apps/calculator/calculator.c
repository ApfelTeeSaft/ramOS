/* calculator.c - Full-featured calculator with +, -, *, / */

#include "../../api/libsys.h"

#define MAX_INPUT 256

/* Calculator state */
static double accumulator = 0.0;
static double operand = 0.0;
static char pending_op = '=';
static int entering_number = 0;
static int error_state = 0;

/* Simple double to string conversion */
static void dtoa(double value, char* buf, int precision) {
    if (value < 0) {
        *buf++ = '-';
        value = -value;
    }
    
    int int_part = (int)value;
    itoa(int_part, buf, 10);
    
    int len = strlen(buf);
    buf[len++] = '.';
    
    double frac_part = value - int_part;
    
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

/* Perform pending operation */
static void execute_operation(void) {
    if (error_state) return;
    
    switch (pending_op) {
        case '+':
            accumulator += operand;
            break;
        case '-':
            accumulator -= operand;
            break;
        case '*':
            accumulator *= operand;
            break;
        case '/':
            if (operand == 0.0) {
                println("Error: Division by zero");
                error_state = 1;
                accumulator = 0.0;
                return;
            }
            accumulator /= operand;
            break;
        case '=':
            accumulator = operand;
            break;
    }
}

/* Display current value */
static void display_value(void) {
    char buf[64];
    
    if (error_state) {
        println("Display: ERROR");
    } else {
        dtoa(accumulator, buf, 6);
        printf("Display: %s\n", buf);
    }
}

/* Clear calculator */
static void clear_calculator(void) {
    accumulator = 0.0;
    operand = 0.0;
    pending_op = '=';
    entering_number = 0;
    error_state = 0;
    println("Cleared");
}

/* Display help */
static void show_help(void) {
    println("\nCalculator Usage:");
    println("  Enter numbers and press Enter");
    println("  Operators: + - * /");
    println("  = or Enter after operator shows result");
    println("\nCommands:");
    println("  c  - Clear");
    println("  h  - Help");
    println("  q  - Quit");
    println("\nExamples:");
    println("  5 [Enter] + [Enter] 3 [Enter] =");
    println("  Result: 8");
    println("");
}

/* Main calculator loop */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[MAX_INPUT];
    
    println("========================================");
    println("  ramOS Calculator v1.0");
    println("========================================");
    show_help();
    
    clear_calculator();
    display_value();
    
    while (1) {
        print("> ");
        readln(input, MAX_INPUT);
        
        /* Skip empty input */
        if (input[0] == '\0') {
            continue;
        }
        
        /* Handle commands */
        if (input[0] == 'q' && input[1] == '\0') {
            println("Goodbye!");
            break;
        } else if (input[0] == 'h' && input[1] == '\0') {
            show_help();
            display_value();
            continue;
        } else if (input[0] == 'c' && input[1] == '\0') {
            clear_calculator();
            display_value();
            continue;
        }
        
        /* Handle operators */
        if ((input[0] == '+' || input[0] == '-' || input[0] == '*' || 
             input[0] == '/' || input[0] == '=') && input[1] == '\0') {
            
            if (entering_number) {
                /* Execute pending operation with current operand */
                execute_operation();
                display_value();
                entering_number = 0;
            }
            
            if (input[0] == '=') {
                pending_op = '=';
            } else {
                pending_op = input[0];
            }
            
            continue;
        }
        
        /* Parse number */
        double value = atof(input);
        operand = value;
        entering_number = 1;
        
        /* Show what was entered */
        char buf[64];
        dtoa(value, buf, 6);
        printf("Entered: %s\n", buf);
    }
    
    sys_exit(0);
    return 0;
}