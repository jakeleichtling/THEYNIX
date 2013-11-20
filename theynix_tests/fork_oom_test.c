/**
  Recursively forks and execs until there is no more memory. Then unwinds.
*/

#include <stdlib.h>
#include <yalnix.h>

int main(int argc, char *argv[]) {
    TtyPrintf(1, "A whole new main!\n");

    char buffer[12345];

    int rc = Fork();
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
    if (GetPid() == 1) {
        rc = Fork();
        if (rc == 0) { // Child process
            // Exec recursively
            rc = Exec("theynix_tests/fork_oom_test", NULL);
            if (rc < 0) {
                TtyPrintf(1, "Exec failed :(\n");
            }
        }

        if (rc < 0) {
            TtyPrintf(1, "Fork failed! How will I start anew?");
        }
    }

    return 0;
}
