#include <hardware.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    int rc;

    // init with invalid id addr
    rc = LockInit((void *) 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "LockInit w/ invalid addr: rc = %d\n", rc);
    

    // Create a single lock that the initial process obtains.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Creating the lock.\n");
    int lock_id;
    rc = LockInit(&lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "LockInit valid addr: rc = %d\n", rc);

    // Try releasing a lock I have not acquired
    rc = Release(lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Release lock I don't hold: rc = %d\n", rc);

    // Try releasing a random id
    rc = Release(128);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Release nonexistant lock: rc = %d\n", rc);

    // Try acquiring some random id
    rc = Acquire(129);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Acquire nonexistant lock: rc = %d\n", rc);

    // The initial process acquires the free lock.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Acquiring the lock.\n");
    rc = Acquire(lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Acquired free lock: rc = %d\n", rc);

    // The initial process then attempts to acquire the lock it holds. This should return immediately.
    rc = Acquire(lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Acquired I already own: rc = %d\n", rc);

    // Spawn 3 processes, each of which waits on the lock.
    int i;
    int n = 3;
    bool is_parent = true;
    for (i = 0; i < n; i++) {
        rc = Fork();

        if (0 == rc) { // Child process
            TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "====I'm a child with PID %d and I WANT the lock!\n", GetPid());

            is_parent = false;

            // lock already held by someone else
            rc = Acquire(lock_id);
            TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "====Acquired lock: rc = %d\n", rc);

            break;
        }
    }

    // When a spawned process acquires the locks, it delays for a while, then releases it.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm process %d and I HAVE the lock!\n", GetPid());

    Delay(4);

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm process %d and I am RELEASING the lock!\n", GetPid());

    // This will test releasing a lock with and without procs waiting
    Release(lock_id);

    int status;
    if (is_parent) {
        for (i = 0; i < n; i++) {
            Wait(&status);
        }

        Reclaim(lock_id);
    }


    return 0;
}
