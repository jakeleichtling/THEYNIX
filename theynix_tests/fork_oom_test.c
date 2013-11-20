/**
  Recursively forks and execs until there is no more memory. Then unwinds.
*/

#include <stdlib.h>
#include <yalnix.h>

int main(int argc, char *argv[]) {
    TtyPrintf(1, "A whole new main!\n");

    char buffer[12345];

    TtyPrintf(1, "About to Fork().\n");
    int rc = Fork();
    TtyPrintf(1, "Fork returned: %d.\n", rc);
    if (rc == 0) { // Child process
        // Exec recursively
        TtyPrintf(1, "Now execing!\n");
        rc = Exec("theynix_tests/fork_oom_test", NULL);
        if (rc < 0) {
            TtyPrintf(1, "Exec failed :(\n");
        }
    }

    if (rc < 0) {
        TtyPrintf(1, "Fork failed! Now I can return.\n");
        return 0;
    }

    int status;
    Wait(&status);
    TtyPrintf(1, "My child returned. Hurray!\n");

    // If I'm the init process, do it again, one more time!
    TtyPrintf(1, "GetPid().\n");
    if (GetPid() == 1) {
        TtyPrintf(1, "Fork().\n");
        rc = Fork();
        TtyPrintf(1, "Fork() returned %d.\n", rc);
        if (rc == 0) { // Child process
            // Exec recursively
            TtyPrintf(1, "Exec().\n");
            rc = Exec("theynix_tests/fork_oom_test", NULL);
            if (rc < 0) {
                TtyPrintf(1, "Exec failed :(\n");
            }
        }

        if (rc < 0) {
            TtyPrintf(1, "Fork failed! How will I start anew?");
        } else {
            Wait(&status);
        }
    }

    return 0;
}
