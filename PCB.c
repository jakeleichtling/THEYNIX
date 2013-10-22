#include "PCB.h"

#include <stdlib.h>

#include "Log.h"

/*
  Returns a PCB with the given model UserContext deep cloned and its lists initialized.
*/
PCB *NewBlankPCB(UserContext model_user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> NewBlankPCB()\n");

    // Malloc for the struct.
    PCB *new_pcb = (PCB *) calloc(1, sizeof(PCB));

    // Deep clone the model user context.
    new_pcb->user_context = model_user_context;

    // Initialize lists.
    new_pcb->live_children = ListNewList();
    new_pcb->zombie_children = ListNewList();

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< NewBlankPCB()\n\n");
    return new_pcb;
}

/*
  Same as above but allocates page tables and frames for kernel stack. Returns NULL if there are
  not enough physical frames to complete this request.
*/
PCB *NewBlankPCBWithPageTables(UserContext model_user_context, UnusedFrames unused_frames) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> NewBlankPCBWithPageTables()\n");

    PCB *pcb = NewBlankPCB(model_user_context);

    // Perform the malloc for the PCB's kernel stack page table.
    current_proc->kernel_stack_page_table =
            (struct pte *) calloc(KERNEL_STACK_MAXSIZE / PAGESIZE, sizeof(struct pte));

    // Create the proc's page table for region 1.
    CreateRegion1PageTable(current_proc);

    // Create the PTEs for the proc's kernel stack with newly allocated frames and
    // the proper protections.
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    unsigned int kernel_stack_limit_page = ADDR_TO_PAGE(KERNEL_STACK_LIMIT - 1) + 1;
    for (i = kernel_stack_base_page; i < kernel_stack_limit_page; i++) {
        int newly_allocated_frame = GetUnusedFrame(unused_frames);
        if (newly_allocated_frame == THEYNIX_EXIT_FAILURE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "GetUnusedFrame() failed.\n");
            return NULL;
        }

        pcb->kernel_stack_page_table[i].pfn = newly_allocated_frame;
        current_proc->kernel_stack_page_table[i].prot = PROT_READ | PROT_WRITE;
        pcb->kernel_stack_page_table[i].valid = 1;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< NewBlankPCBWithPageTables()\n");
    return pcb;
}

void KillCurrentProc() {
    // TODO
}
