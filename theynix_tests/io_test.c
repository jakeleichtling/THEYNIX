#include <stdlib.h>
#include <stdio.h>
#include <yalnix.h> // for printing
#include <string.h>
#include <hardware.h>

int main(int argc, char *argv[]) {
    char *hello_message = "Hello, I'm a THEYNIX process!\n\0";
    TtyWrite(0, hello_message, strlen(hello_message));

    char *really_long_line = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH\n\0";
    TtyWrite(0, really_long_line, strlen(really_long_line));

    char *user_message = calloc(TERMINAL_MAX_LINE, sizeof(char));
    TtyWrite(0, "Please input a message\n", TERMINAL_MAX_LINE);
    int input_length = TtyRead(0, user_message, TERMINAL_MAX_LINE);
    TtyWrite(0, user_message, input_length);

    int failure_code = TtyWrite(0, NULL, TERMINAL_MAX_LINE);
    char *failure_test_message = calloc(128, sizeof(char));
    sprintf(failure_test_message, "Tried to write null. rc = %d\n", failure_code);
    TtyWrite(0, failure_test_message, 128);

    int rc = Fork();
    if (0 == rc) {
        bzero(user_message, TERMINAL_MAX_LINE);
        TtyWrite(0, "Now I (the child) will read!\n", TERMINAL_MAX_LINE);
        input_length = TtyRead(0, user_message, TERMINAL_MAX_LINE);
        TtyWrite(0, user_message, input_length);
        exit(0);
    }
    TtyWrite(0, "I'm trying to print at the same time as my child!\n", TERMINAL_MAX_LINE);
    int status;
    Wait(&status); // Let child die first
    return 0;
}
