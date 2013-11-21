/**
  This program forks and execs until a child's PID > n. Each parent waits on its child, then prints out the exit status of the child.
  Then the parent prints its own PID and exits with the exit status of its PID.
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int n = 11;

int main(int argc, char **argv) {
    int pid = GetPid();
    int rc;
    int status;

    if (pid <= n) {
        rc = Fork();

        if (rc < 0) {
            TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Fork failed. My PID is %d.\n", pid);
            Exit(-1);
        } else if (rc == 0) {
            Exec("theynix_tests/child_chain", NULL);
        }

        Wait(&status);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "My child (%d) exited with status %d.\n", rc, status);
    }

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I am exiting: %d\n", pid);
    Exit(pid);
}
