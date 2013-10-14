#include <assert.h>

#include "Kernel.h"

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
    kernel_brk_page = ADDR_TO_PAGE(((unsigned int) _KernelDataEnd) - 1) + 1;
    kernel_data_start_page = ADDR_TO_PAGE(_KernelDataStart);
    kernel_text_end_page = ADDR_TO_PAGE(((unsigned int) _KernelDataStart) - 1);
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
    // Initialize the kernel's data structures.
    locks = ListNewList();
    cvars = ListNewList();
    pipes = ListNewList();

    ttys = (Tty *) malloc(NUM_TERMINALS * sizeof(Tty));
    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        TtyInit(&ttys[i]);
    }

    //current_proc =
    ready_queue = ListNewList();
    clock_block_procs = ListNewList();

    unused_frames = NewUnusedFrames(pmem_size, kernel_brk_page);
    virtual_memory_enabled = false;

    // Initialize the interrupt vector table.

    // Initialize the REG_VECTOR_BASE register to point to the interrupt vector table.

    // Build the initial page table for regions 0 and 1 such that physical address =
    // virtual address for all used frames, and initialize REG_PTBR0,
    // REG_PTLR0, REG_PTBR1, and REG_PTLR1

    // Enable virtual memory. Wooooo!
    WriteRegister(REG_VM_ENABLE, 1);
    virtual_memory_enabled = true;

    // Create the idle process and put it on the ready queue.

    // Create the first process (see template.c) and load the initial program into it.
}

int SetKernelBrk(void *addr) {
    // Ensure we aren't imposing on kernel stack limits, otherwise return -1

    // Get da frames!

    // Increase kernel_break_page

    // Return 0
}
