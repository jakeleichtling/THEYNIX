#ifndef _LOCK_H_
#define _LOCK_H_

/*
  Code for mutual exclusion locks.
*/

struct Lock {
    int id;
    int owner_id;

    bool acquired;
};

typedef struct Lock Lock;

#endif
