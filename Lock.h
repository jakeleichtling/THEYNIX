/*
  Code for mutual exclusion locks.
*/

struct Lock {
    int id;
    int owner_id;

    bool acquired;
};

typedef struct Lock Lock;
