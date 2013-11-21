/**
  This program tests that the OS properly handles bad wait calls:
  - Illegal status pointer argument
  - No children
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int main(int argc, char **argv) {
    int rc = Fork();
    if (rc < 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Fork failed. My PID is %d.\n", GetPid());
        Exit(-1);
    }
    if (rc == 0) {
        Exit(1337);
    }

    // Wait with illegal status pointer arg
    int *bad_status_ptr = (int *) 0;
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Waiting with illegal status pointer arg...\n");
    rc = Wait(bad_status_ptr);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Wait on my child correctly.
    int status;
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Waiting correctly...\n");
    rc = Wait(&status);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> Child exited with %d\n", status);

    // Waiting again now that I have no children.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Waiting with no children left...\n");
    rc = Wait(&status);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "All done!\n", rc);

    return SUCCESS;
}
