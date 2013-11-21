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

    // Trying to malloc more than the amount of physical memory available.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Brking close to the stack.\n");
    rc = Brk(buffer - 2048);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "All done!\n");
    return SUCCESS;
}
