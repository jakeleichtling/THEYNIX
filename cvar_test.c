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

    int child_cvar_id;
    CvarInit(&child_cvar_id);

    int parent_cvar_id;
    CvarInit(&parent_cvar_id);

    // The initial process then attempts to acquire the lock it holds. This should return immediately.
    // Spawn 3 processes, each of which waits on the lock.
    int rc;
    int i;
    int n = 3;
    bool is_parent = true;
    for (i = 0; i < n; i++) {
        rc = Fork();

        if (0 == rc) { // Child process
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm a child with PID %d and I WANT the lock!\n",
                 GetPid());

            is_parent = false;

            Acquire(lock_id);

            break;
        }
    }

    // When a spawned process acquires the locks, it delays for a while, then releases it.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm process %d and I HAVE the lock!\n", GetPid());

    if (!is_parent) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm a child w/ PID %d and I'm waiting on my cvar!\n",
            GetPid());
        CvarWait(child_cvar_id, lock_id);
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'ma a child w/ PID %d and I've finished waiting!\n",
            GetPid());

        CvarSignal(parent_cvar_id);
    } else {
        Delay(5); // let children start waiting
        Acquire(lock_id);
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm parent w/ PID %d and I'm broadcasting for child  cvar!\n",
            GetPid());

        CvarBroadcast(child_cvar_id);
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm parent w/ PID %d and I'm waiting on my cvar!\n",
            GetPid());
        CvarWait(parent_cvar_id, lock_id);
    }
    Release(lock_id);
    
    int status;
    if (is_parent) {
        for (i = 0; i < n; i++) {
            Wait(&status);
        }
    }

    return 0;
}
