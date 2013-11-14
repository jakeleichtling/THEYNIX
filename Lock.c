#include "Lock.h"

#include <stdlib.h>

#include "Kernel.h"

extern unsigned int next_synch_resource_id;

/*
  Constructs a new lock with default fields.
*/
Lock *LockNewLock() {
    Lock *lock = calloc(1, sizeof(Lock));

    lock->id = next_synch_resource_id++;
    lock->acquired = false;

    lock->waiting_procs = ListNewList();

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
