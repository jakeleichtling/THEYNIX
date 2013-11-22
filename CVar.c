#include "CVar.h"

#include <stdlib.h>

#include "Kernel.h"

extern unsigned int next_synch_resource_id;

/*
  Constructs a new cvar with default fields.
*/
CVar *CVarNewCVar() {
    CVar *cvar = calloc(1, sizeof(CVar));

    cvar->id = next_synch_resource_id++;
    cvar->waiting_procs = ListNewList(0);

    return cvar;
}

/*
  Free the cvar.

  The list of waiting processes must be empty.
*/
void CVarDestroy(CVar *cvar) {
    ListDestroy(cvar->waiting_procs);

    free(cvar);
}
