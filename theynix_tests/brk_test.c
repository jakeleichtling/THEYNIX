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
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Brk-ing into the stack.\n");
    int rc = Brk(buffer);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Brk near the stack.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Brk-ing close to the stack.\n");
    rc = Brk(buffer - 4096);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Now Brk down a bit.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Now Brki-ng down a bit.\n");
    rc = Brk(buffer - 4096 * 2);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "All done!\n");
    return SUCCESS;
}
