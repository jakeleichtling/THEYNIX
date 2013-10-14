/*
  Code for condition variables.
*/

#include "PCB.h"

struct CVar {
    int id;

    List *waiting_procs;
};

typedef struct Lock Lock;
