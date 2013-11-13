#include <hardware.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    // Create a single lock that the initial process obtains.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Creating the lock.\n");
    int lock_id;
    LockInit(&lock_id);

    // The initial process acquires the lock.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Acquiring the lock.\n");
    Acquire(lock_id);

    // The initial process then attempts to acquire the lock it holds. This should return immediately.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Acquiring the lock... again!\n");
    Acquire(lock_id);

    // Spawn 5 processes, each of which waits on the lock.
    int rc;
    int i;
    int n = 5;
    bool is_parent = true;
    for (i = 0; i < n; i++) {
        rc = Fork();

        if (0 == rc) { // Child process
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm a child with PID %d and I WANT the lock!\n", GetPid());

            is_parent = false;

            Acquire(lock_id);

            break;
        }
    }

    // When a spawned process acquires the locks, it delays for a while, then releases it.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm process %d and I HAVE the lock!\n", GetPid());

    Delay(4);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm process %d and I am RELEASING the lock!\n", GetPid());

    Release(lock_id);

    int status;
    if (is_parent) {
        for (i = 0; i < n; i++) {
            Wait(&status);
        }
    }

    return 0;
}
