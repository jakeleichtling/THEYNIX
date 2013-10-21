#include "PCB.h"

#include <stdlib.h>

/*
  Returns a PCB with the given model UserContext deep cloned and its lists initialized.
*/
PCB *NewBlankPCB(UserContext model_user_context) {
    // Malloc for the struct.
    PCB *new_pcb = (PCB *) calloc(1, sizeof(PCB));

    // Deep clone the model user context.
    new_pcb->user_context = model_user_context;

    // Initialize lists.
    new_pcb->live_children = ListNewList();
    new_pcb->zombie_children = ListNewList();

    return new_pcb;
}

void KillCurrentProc() {
    // TODO
}
