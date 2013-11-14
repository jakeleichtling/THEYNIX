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

/*
  Free the cvar.

  The list of waiting processes must be empty.
*/
void CVarDestroy(CVar *cvar);

#endif
