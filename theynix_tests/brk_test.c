/**
  This program tests the Brk() syscall.
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    int buffer[1024];

    // Try to set the brk in the stack.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Brking into the stack.\n");
    int rc = Brk(buffer);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);


    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "All done!\n");
    return SUCCESS;
}
