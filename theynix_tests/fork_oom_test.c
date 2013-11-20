/**
  Recursively forks and execs until there is no more memory. Then unwinds.
*/

#include <yalnix.h>

int main(int argc, char *argv[]) {
    int rc = Fork();
    if (rc == 0) { // Child process
        // Exec recursively
        Exec("fork_oom_test", NULL);
    }

    if (rc < 0) {
        TtyPrintf(1, "Fork failed! Now I can return.");
        return;
    }

    Wait();
    TtyPrintf(1, "My child returned. Hurray!");
}
