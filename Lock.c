#include "Lock.h"

#include <stdlib.h>

#include "Kernel.h"

/*
 * Lock.c
 * Data structure for mutexes
 */

extern unsigned int next_synch_resource_id;

/*
  Constructs a new lock with default fields.
*/
Lock *LockNewLock() {
    Lock *lock = calloc(1, sizeof(Lock));

    lock->id = next_synch_resource_id++;
    lock->acquired = false;

    // Only look up by ID when a proc dies
    // while holding the lock, which won't happen
    // too often
    lock->waiting_procs = ListNewList(0);

    return lock;
}

/*
  Free the lock.

  The list of waiting processes must be empty.
*/
void LockDestroy(Lock *lock) {
    ListDestroy(lock->waiting_procs);

    free(lock);
}
