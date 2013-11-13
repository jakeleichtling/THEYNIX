#ifndef _LOCK_H_
#define _LOCK_H_

#import "List.h"

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

#endif
