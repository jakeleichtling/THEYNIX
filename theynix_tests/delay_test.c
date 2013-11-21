/**
  This program tests the Delay() syscall.
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int main(int argc, char **argv) {
    // Delaying with ticks < 0
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Delaying with ticks < 0...\n");
    int rc = Delay(-17);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Delaying with ticks == 0
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Delaying with ticks == 0...\n");
    rc = Delay(0);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Delaying with ticks == 9
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Delaying with ticks == 9...\n");
    rc = Delay(9);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    return SUCCESS;
}
