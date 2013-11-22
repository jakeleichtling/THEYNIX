#include <hardware.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    // Create a single lock that the initial process obtains.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Creating the lock.\n");
    int lock_id;
    int parent_cvar_id;
    LockInit(&lock_id);


    int rc;
    // Init with invalid id pointer
    rc = CvarInit((void *) 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarInit w/ invalid pointer: rc = %d\n", rc);

    // Signal with bad id
    rc = CvarSignal(2388);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarSignal w/ invalid id: rc = %d\n", rc);

    // Broadcast with bad id
    rc = CvarBroadcast(2388);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarBroadcast w/ invalid id: rc = %d\n", rc);

    // Wait w/ bad cvar id
    rc = CvarWait(lock_id, 1239988);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarWait w/ invalid cvar id: rc = %d\n", rc);

    // Wait w/ lock I don't own
    rc = CvarWait(lock_id, parent_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarWait w/ lock I don't own: rc = %d\n", rc);

    // Wait with bad lock id
    rc = CvarWait(1283, parent_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarWait w/ invalid cvar id: rc = %d\n", rc);

    int child_cvar_id;
    rc = CvarInit(&child_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarInit w/ %x: rc = %d\n", &child_cvar_id, rc);

    // Signal with no one waiting
    rc = CvarSignal(child_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarSignal w/ no waiting proc rc = %d\n", rc);

    // Broadcast with no one waiting
    rc = CvarBroadcast(child_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarBroadcast w/ no waiting proc rc = %d\n", rc);

    rc = CvarInit(&parent_cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "CvarInit w/ %x: rc = %d\n", &parent_cvar_id, rc);

    // The initial process then attempts to acquire the lock it holds. This should return immediately.
    // Spawn 3 processes, each of which waits on the lock.
    int i;
    int n = 3;
    bool is_parent = true;
    for (i = 0; i < n; i++) {
        rc = Fork();

        if (0 == rc) { // Child process
            TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm a child with PID %d and I WANT the lock!\n",
                 GetPid());

            is_parent = false;

            Acquire(lock_id);

            break;
        }
    }

    // When a spawned process acquires the locks, it delays for a while, then releases it.
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm process %d and I HAVE the lock!\n", GetPid());

    if (!is_parent) {
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm a child w/ PID %d and I'm waiting on my cvar!\n",
            GetPid());
        CvarWait(child_cvar_id, lock_id);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'ma a child w/ PID %d and I've finished waiting!\n",
            GetPid());

        CvarSignal(parent_cvar_id); // signal with proc waiting
    } else {
        Delay(5); // let children start waiting
        Acquire(lock_id);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm parent w/ PID %d and I'm broadcasting for child  cvar!\n",
            GetPid());

        CvarBroadcast(child_cvar_id); // Broadchat with procs waiting
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm parent w/ PID %d and I'm waiting on my cvar!\n",
            GetPid());
        CvarWait(parent_cvar_id, lock_id);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm parent w/ PID %d and I've finished waiting!!\n",
            GetPid());
    }
    Release(lock_id);
    
    int status;
    if (is_parent) {
        for (i = 0; i < n; i++) {
            Wait(&status);
        }
        Reclaim(lock_id);
        Reclaim(child_cvar_id);
        Reclaim(parent_cvar_id);
    }

    return 0;
}
