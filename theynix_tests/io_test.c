#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <yalnix.h> // for printing
#include <string.h>
#include <hardware.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    // Test normal write behavior.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing normal write behavior.\n");
    char *hello_message = "Hello, I'm a THEYNIX process!\n";
    TtyWrite(0, hello_message, strlen(hello_message));

    // Test normal read behavior.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing normal read behavior.\n");
    char *user_message = calloc(TERMINAL_MAX_LINE, sizeof(char));
    char *prompt_input_message = "Please input a message\n";
    TtyWrite(0, prompt_input_message, strlen(prompt_input_message));
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "About to call TtyRead()\n");
    int input_length = TtyRead(0, user_message, TERMINAL_MAX_LINE);
    TtyWrite(0, user_message, input_length);

    // Test writing a line longer than TERMINAL_MAX_LINE.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing writing a line longer than TERMINAL_MAX_LINE.\n");
    char *really_long_line = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH\n\0";
    int num_written = TtyWrite(0, really_long_line, strlen(really_long_line));
    assert(num_written == strlen(really_long_line));

    // Test reading with length greater than TERMINAL_MAX_LINE.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing reading a line longer than TERMINAL_MAX_LINE.\n");
    char *big_buffer = calloc(TERMINAL_MAX_LINE * 2, sizeof(char));
    int num_read = TtyRead(0, big_buffer, TERMINAL_MAX_LINE * 2);
    assert(num_read <= TERMINAL_MAX_LINE);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "%d characters were read.\n", num_read);

    // Test writing with invalid (null) buffer.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing writing with an invalid buffer.\n");
    int failure_code = TtyWrite(0, NULL, TERMINAL_MAX_LINE);
    char *failure_test_message = calloc(128, sizeof(char));
    sprintf(failure_test_message, "Tried to write null. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test reading with invalid (null) buffer.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing reading with an invalid buffer.\n");
    failure_code = TtyRead(0, NULL, TERMINAL_MAX_LINE);
    bzero(failure_test_message, 128);
    sprintf(failure_test_message, "Tried to read into null buffer. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test writing with id < 0
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing writing with id < 0.\n");
    failure_code = TtyWrite(-1, big_buffer, TERMINAL_MAX_LINE);
    bzero(failure_test_message, 128);
    sprintf(failure_test_message, "Tried to write with id < 0. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test reading with id < 0
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing reading with id < 0.\n");
    failure_code = TtyRead(-1, big_buffer, TERMINAL_MAX_LINE);
    bzero(failure_test_message, 128);
    sprintf(failure_test_message, "Tried to read with id < 0. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test writing with id > 3
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing writing with id > 3.\n");
    failure_code = TtyWrite(4, big_buffer, TERMINAL_MAX_LINE);
    bzero(failure_test_message, 128);
    sprintf(failure_test_message, "Tried to write with id > 3. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test reading with id > 3
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Testing reading with id > 3.\n");
    failure_code = TtyRead(4, big_buffer, TERMINAL_MAX_LINE);
    bzero(failure_test_message, 128);
    sprintf(failure_test_message, "Tried to read with id > 3. rc = %d\n\0", failure_code);
    TtyWrite(0, failure_test_message, strlen(failure_test_message));

    // Test having two writers at the same time.
    int rc = Fork();
    if (0 == rc) {
        bzero(user_message, TERMINAL_MAX_LINE);
        char *child_read_msg = "Now I (the child) will read!\n";
        TtyWrite(0, child_read_msg, strlen(child_read_msg));
        input_length = TtyRead(0, user_message, TERMINAL_MAX_LINE);
        TtyWrite(0, user_message, input_length);
        exit(0);
    }
    char *same_time_msg = "I'm trying to print at the same time as my child!\n";
    TtyWrite(0, same_time_msg, strlen(same_time_msg));
    int status;
    Wait(&status); // Let child die first
    return 0;
}
