#include "Kernel.h"

#include <assert.h>
#include <stdlib.h>

#include "Log.h"
#include "Traps.h"
#include "VMem.h"

/* Function Prototypes */

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewFrame(unsigned int page_number);

/*
  Unmaps a valid page, freeing the frame the page was mapped to. Returns -1 on failure.
*/
void UnmapUsedFrame(unsigned int page_number);

/*
  Copies the given kernel stack page table into the region 0 page table. Does not flush the TLB.
*/
void UseKernelStackForProc(PCB *pcb);

/*
  Infinite loop that calls Pause() on each iteration.
*/
void Idle();

/* Function Implementations */

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
    kernel_brk_page = ADDR_TO_PAGE(((unsigned int) _KernelDataEnd) - 1) + 1;
    kernel_data_start_page = ADDR_TO_PAGE(_KernelDataStart);
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
    // Initialize the kernel's data structures.
    locks = ListNewList();
    cvars = ListNewList();
    pipes = ListNewList();

    ttys = (Tty *) malloc(NUM_TERMINALS * sizeof(Tty));
    unsigned int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        TtyInit(&ttys[i]);
    }

    ready_queue = ListNewList();
    clock_block_procs = ListNewList();

    unused_frames = NewUnusedFrames(pmem_size);
    virtual_memory_enabled = false;

    // Initialize the interrupt vector table and write the base address
    // to the REG_VECTOR_BASE register
    TrapTableInit();

    // Create the current process
    current_proc = NewBlankPCB(uctxt);

    // Perform the malloc for the current proc's kernel stack page table before making page tables.
    current_proc->kernel_stack_page_table =
            (struct pte *) malloc(KERNEL_STACK_MAXSIZE * sizeof(struct pte));

    // Build the initial page table for region 0 such that page = frame for all valid pages.
    region_0_page_table = (struct pte *) malloc(VMEM_0_SIZE / PAGESIZE * sizeof(struct pte));

    // Clear the valid bit of all PTEs.
    for (i = 0; i < VMEM_0_SIZE / PAGESIZE; i++) {
        region_0_page_table[i].valid = 0;
    }

    // Create the proc's page table for region 1.
    CreateRegion1PageTable(current_proc);

    // Create the PTEs for the kernel text and data with the proper protections.
    for (i = 0; i < kernel_brk_page; i++) {
        region_0_page_table[i].valid = 1;
        region_0_page_table[i].pfn = i;
        MarkFrameAsUsed(unused_frames, i);

        if (i < kernel_data_start_page) { // Text section.
            region_0_page_table[i].prot = PROT_READ | PROT_EXEC;
        } else { // Data section.
            region_0_page_table[i].prot = PROT_READ | PROT_WRITE;
        }
    }

    // Create the PTEs for the proc's kernel stack with page = frame and the proper protections.
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    unsigned int kernel_stack_limit_page = ADDR_TO_PAGE(KERNEL_STACK_LIMIT - 1) + 1;
    for (i = kernel_stack_base_page; i < kernel_stack_limit_page; i++) {
        current_proc->kernel_stack_page_table[i].valid = 1;
        current_proc->kernel_stack_page_table[i].pfn = i;
        MarkFrameAsUsed(unused_frames, i);

        current_proc->kernel_stack_page_table[i].prot = PROT_READ | PROT_WRITE;
    }
    UseKernelStackForProc(current_proc);

    // Set the TLB registers for the region 0 page table.
    WriteRegister(REG_PTBR0, (unsigned int) region_0_page_table);
    WriteRegister(REG_PTLR0, VMEM_0_SIZE / PAGESIZE);

    // Set the TLB registers for the region 1 page table.
    WriteRegister(REG_PTBR1, (unsigned int) current_proc->region_1_page_table);
    WriteRegister(REG_PTLR1, VMEM_1_SIZE / PAGESIZE);

    // Enable virtual memory. Wooooo!
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Enabling virtual memory. Wooooo!");
    virtual_memory_enabled = true;
    WriteRegister(REG_VM_ENABLE, 1);

    // Make the current process the Idle process.
    current_proc->user_context->pc = &Idle;
    *uctxt = *(current_proc->user_context);

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

    // If virtual memory is enabled, give the kernel heap more frames or take some away.
    if (virtual_memory_enabled) {
        unsigned int kernel_stack_base_frame = ADDR_TO_PAGE(KERNEL_STACK_BASE);
        if (new_kernel_brk_page > kernel_brk_page) {
            unsigned int new_page;
            for (new_page = kernel_brk_page;
                    new_page < new_kernel_brk_page && new_page < kernel_stack_base_frame;
                    new_page++) {
                int rc = MapNewFrame(new_page);
                if (rc == THEYNIX_EXIT_FAILURE) {
                    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                            "MapNewFrame(%u) failed.\n", new_page);
                    return THEYNIX_EXIT_FAILURE;
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
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SetKernelBrk()\n\n");
    kernel_brk_page = new_kernel_brk_page;
    return 0;
}

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewFrame(unsigned int page_number) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewFrame(%u)\n", page_number);

    assert(page_number < VMEM_0_LIMIT / PAGESIZE);

    int new_frame = GetUnusedFrame(unused_frames);
    if (new_frame == THEYNIX_EXIT_FAILURE) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "GetUnusedFrame() failed.\n");
        return THEYNIX_EXIT_FAILURE;
    }

    assert(!region_0_page_table[page_number].valid);

    region_0_page_table[page_number].valid = 1;
    region_0_page_table[page_number].pfn = new_frame;
    region_0_page_table[page_number].prot = PROT_READ | PROT_WRITE;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewFrame()\n\n");
    return THEYNIX_EXIT_SUCCESS;
}

/*
  Unmaps a valid page, freeing the frame the page was mapped to.
*/
void UnmapUsedFrame(unsigned int page_number) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> UnmapUsedFrame(%u)\n", page_number);

    assert(page_number < VMEM_0_LIMIT / PAGESIZE);
    assert(region_0_page_table[page_number].valid);

    unsigned int used_frame = region_0_page_table[page_number].pfn;
    ReleaseUsedFrame(unused_frames, used_frame);

    region_0_page_table[page_number].valid = 0;
    WriteRegister(REG_TLB_FLUSH, (unsigned int) &region_0_page_table[page_number]);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< UnmapUsedFrame()\n\n");
}

/*
  Copies the given kernel stack page table into the region 0 page table. Does not flush the TLB.
*/
void UseKernelStackForProc(PCB *pcb) {
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    unsigned int kernel_stack_limit_page = ADDR_TO_PAGE(KERNEL_STACK_LIMIT - 1) + 1;
    unsigned int i;
    for (i = kernel_stack_base_page; i < kernel_stack_limit_page; i++) {
        region_0_page_table[i] = pcb->kernel_stack_page_table[i];
    }
}

/*
  Infinite loop that calls Pause() on each iteration.
*/
void Idle() {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> Idle()\n");
    while (true) {
        Pause();
    }
}
