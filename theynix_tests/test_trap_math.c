/**
  This program tests that a TRAP_MATH trap kills the current process
  but continues running other processes.
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
        int x = 5 / 0;
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "THIS LINE SHOULD NEVER BE PRINTED.\n", GetPid());
    }

    int status;
    Wait(&status);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Child %d printed with status %d.\n", rc, status);

    return SUCCESS;
}
