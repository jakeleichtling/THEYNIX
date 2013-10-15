#include <assert.h>

#include "Kernel.h"
#include "Log.h"

/* Function Prototypes */

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewFrame(unsigned int page_numer);

/*
  Unmaps a valid page, freeing the frame the page was mapped to. Returns -1 on failure.
*/
int UnmapUsedFrame(unsigned int page_number);

/* Function Implementations */

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
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SetKernelBrk(%p)\n", addr);

    unsigned int new_kernel_brk_page = ADDR_TO_PAGE(addr - 1) + 1;

    // Ensure we aren't imposing on kernel stack limits.
    if (((unsigned int) addr) > KERNEL_STACK_BASE) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Address passed to SetKernelBrk() (%p) is greater than kernel stack base (%p).\n",
                addr, KERNEL_STACK_BASE);
        return -1;
    }

    // Give the kernel heap more frames or take some away.
    unsigned int kernel_stack_base_frame = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    if (new_kernel_brk_page > kernel_brk_page) {
        unsigned int new_page;
        for (new_page = kernel_brk_page;
                new_page < new_kernel_brk_page && new_page < kernel_stack_base_frame;
                new_page++) {
            int rc = MapNewFrame(new_page);
            if (rc == EXIT_FAILURE) {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                        "MapNewFrame(%u) failed.", new_page);
                return EXIT_FAILURE;
            }
        }
    } else if (new_kernel_brk_page < kernel_brk_page) {
        unsigned int page_to_free;
        for (page_to_free = kernel_brk_page - 1;
                page_to_free >= new_kernel_brk_page;
                page_to_free--)
            if (page_to_free < kernel_stack_base_frame) {
                UnmapUsedFrame(page_to_free);
            }
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SetKernelBrk()\n\n");
    kernel_brk_page = new_kernel_brk_page;
    return 0;
}

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewFrame(unsigned int page_numer) {
    // TODO
}

/*
  Unmaps a valid page, freeing the frame the page was mapped to. Returns -1 on failure.
*/
int UnmapUsedFrame(unsigned int page_number) {
    // TODO
}
