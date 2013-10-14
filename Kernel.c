#include "Kernel.h"

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
    kernel_brk_page = UP_TO_PAGE(_KernelDataEnd);
    kernel_data_start_page = DOWN_TO_PAGE(_KernelDataStart);
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
    // Initialize data structures in kernel.h, including used_frames

    // Initialize the interrupt vector and point to it in REG_VECTOR_BASE

    // Build the initial page table for region 0 and region 1 such that physical address =
    // virtual address for all used frames, and initialize REG_PTBR0,
    // REG_PTLR0, REG_PTBR1, and REG_PTLR1

    // Enable virtual memory. Wooooo!

    // Create the idle process and put it on the ready queue.

    // Create the first process (see template.c) and load the initial program into it.

    // Return!
}

int SetKernelBrk(void *addr) {
    // Ensure we aren't imposing on kernel stack limits, otherwise return -1

    // Get da frames!

    // Increase kernel_break_page

    // Return 0
}
