#ifndef _CVAR_H_
#define _CVAR_H_

/*
  Code for condition variables.
*/

#include "PCB.h"

struct CVar {
    int id;

    List *waiting_procs;
};

typedef struct CVar CVar;

/*
  Constructs a new cvar with default fields.
*/
CVar *CVarNewCVar();

#endif
