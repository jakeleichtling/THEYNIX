/*
  Tests that the subtlities of Exit() with the following scenario. The init process Forks() and
  then delays n ticks so that the scenario can complete before the machine halts. The first
  child then inits a lock and forks two children. The first child exits immediately and becomes
  a zombie. The second waits on the lock. Then the child of init exits while still holding the lock.
  The child waiting on the lock should then complete, and finally init should terminate.
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int main(int argc, char **argv) {
    int rc = Fork();
    if (rc < 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Fork failed. My PID is %d.\n", GetPid());
    }
    if (rc > 0) { // Init process
        Delay(10);
        Exit(SUCCESS);
    }

    int lock;
    rc = LockInit(&lock);
    if (rc < 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "LockInit failed. Exiting immediately. My PID is %d.\n", GetPid());
        Exit(ERROR);
    }

    rc = Fork();
    if (rc < 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Fork failed. My PID is %d.\n", GetPid());
    }
    if (rc == 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "%d : I'm returning immediately to become a zombie.\n", GetPid());
        Exit(SUCCESS);
    }

    rc = Fork();
    if (rc < 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Fork failed. My PID is %d.\n", GetPid());
    }
    if (rc == 0) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "%d : I'm waiting on the lock.\n", GetPid());
        Acquire(lock);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "%d : Got the lock!\n", GetPid());
        Exit(SUCCESS);
    }

    return SUCCESS;
}
