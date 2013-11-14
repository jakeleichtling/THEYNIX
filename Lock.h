#ifndef _LOCK_H_
#define _LOCK_H_

#include "List.h"

/*
  Code for mutual exclusion locks.
*/

struct Lock {
    int id;
    int owner_id;

    List *waiting_procs;

    bool acquired;
};

typedef struct Lock Lock;

/*
  Constructs a new lock with default fields.
*/
Lock *LockNewLock();

/*
  Free the lock.

  The list of waiting processes must be empty.
*/
void LockDestroy(Lock *lock);

#endif
