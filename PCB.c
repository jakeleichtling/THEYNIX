/*
  Returns a PCB with the given model UserContext deep cloned and its lists initialized.
*/
PCB *NewBlankPCB(UserContext *model_user_context) {
    // Malloc for the struct.
    PCB *new_pcb = (PCB *) malloc(sizeof(PCB));

    // Deep clone the model user context.
    new_pcb->user_context = (UserContext *) malloc(sizeof(UserContext));
    *(new_pcb->user_context) = *model_user_context;

    // Initialize lists.
    new_pcb->live_children = ListNewList();
    new_pcb->zombie_children = ListNewList();
}
