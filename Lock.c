#include "Lock.h"

#include "Kernel.h"

extern unsigned int next_synch_resource_id;

/*
  Constructs a new lock with default fields.
*/
Lock *LockNewLock() {
    Lock *lock = calloc(1, sizeof(Lock));

    lock->id = next_synch_resource_id++;
    lock->acquired = false;

    waiting_procs = ListNewList();

    return lock;
}
