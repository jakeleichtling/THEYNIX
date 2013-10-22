#include "Kernel.h"

#include <assert.h>
#include <stdlib.h>

#include "Log.h"
#include "Traps.h"
#include "VMem.h"

/* Function Prototypes */

/*
  Copies the given kernel stack page table into the region 0 page table. Does not flush the TLB.
*/
void UseKernelStackForProc(PCB *pcb);

/*
  Infinite loop that calls Pause() on each iteration.
*/
void Idle();

/*
  Allocate some Kernel Data structures for Tty, process, and synchronization bookkeeping.
*/
void InitBookkeepingStructs();

/* Function Implementations */

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd) {
    kernel_brk_page = ADDR_TO_PAGE(((unsigned int) _KernelDataEnd) - 1) + 1;
    kernel_data_start_page = ADDR_TO_PAGE(_KernelDataStart);
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
    unused_frames = NewUnusedFrames(pmem_size);
    virtual_memory_enabled = false;

    // Initialize the interrupt vector table and write the base address
    // to the REG_VECTOR_BASE register
    TrapTableInit();

    // Create the current process
    current_proc = NewBlankPCB(*uctxt);

    // Perform the malloc for the current proc's kernel stack page table before making page tables.
    current_proc->kernel_stack_page_table =
            (struct pte *) calloc(KERNEL_STACK_MAXSIZE / PAGESIZE, sizeof(struct pte));

    // Build the initial page table for region 0 such that page = frame for all valid pages.
    region_0_page_table = (struct pte *) calloc(VMEM_0_SIZE / PAGESIZE, sizeof(struct pte));

    // Clear the valid bit of all PTEs.
    // ---> I don't think we need this if calloc
    unsigned int i = 0;
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
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Enabling virtual memory. Wooooo!\n");
    virtual_memory_enabled = true;
    WriteRegister(REG_VM_ENABLE, 1);

    // Get one valid page at the top of region 1 for the user stack of the current proc.
    unsigned int first_stack_frame = GetUnusedFrame(unused_frames);
    unsigned int top_pte_index = ADDR_TO_PAGE(VMEM_1_LIMIT - 1);
    struct pte *top_pte_ptr = &(current_proc->region_1_page_table[top_pte_index]);
    top_pte_ptr->valid = 1;
    top_pte_ptr->prot = PROT_READ | PROT_WRITE;
    top_pte_ptr->pfn = first_stack_frame;

    // Make the current process the Idle process.
    current_proc->user_context.pc = &Idle;

    // We think we need to give the sp some valid address,
    // so just hack it by pointing to some buffer.
    UserContext *args_buffer = calloc(2, sizeof(UserContext));
    current_proc->user_context.sp = args_buffer;

    *uctxt = current_proc->user_context;

    InitBookkeepingStructs();

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
                int rc = MapNewRegion0Page(new_page, unused_frames);
                if (rc == THEYNIX_EXIT_FAILURE) {
                    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                            "MapNewRegion0Page(%u) failed.\n", new_page);
                    return THEYNIX_EXIT_FAILURE;
                }
            }
        } else if (new_kernel_brk_page < kernel_brk_page) {
            unsigned int page_to_free;
            for (page_to_free = kernel_brk_page - 1;
                    page_to_free >= new_kernel_brk_page;
                    page_to_free--)
                if (page_to_free < kernel_stack_base_frame) {
                    UnmapUsedRegion0Page(page_to_free, unused_frames);
                }
        }
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< Brk()\n\n");
    kernel_brk_page = new_kernel_brk_page;
    return 0;
}

int Brk(void *addr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> Brk(%p)\n", addr);

    unsigned int new_user_brk_page = ADDR_TO_PAGE(addr - 1) + 1;

    // Ensure we aren't imposing on user stack limits.
    if (new_user_brk_page >= current_proc->lowest_user_stack_page) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Address passed to Brk() (%p) does not leave a blank page between heap and user stack page (%d).\n",
                new_user_brk_page, current_proc->lowest_user_stack_page);
        return THEYNIX_EXIT_FAILURE;
    }

    if (new_user_brk_page > current_proc->user_brk_page) {
        int rc = MapNewRegion1Pages(current_proc, unused_frames, current_proc->user_brk_page,
                new_user_brk_page - current_proc->user_brk_page, PROT_READ | PROT_WRITE);
        if (rc == THEYNIX_EXIT_FAILURE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "MapNewRegion1Pages() failed.\n");
            return THEYNIX_EXIT_FAILURE;
        }
    } else if (new_user_brk_page < current_proc->user_brk_page) {
        UnmapRegion1Pages(current_proc, unused_frames, new_user_brk_page,
                current_proc->user_brk_page - new_user_brk_page);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< Brk()\n\n");
    current_proc->user_brk_page = new_user_brk_page;
    return 0;
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

void InitBookkeepingStructs() {
    locks = ListNewList();
    cvars = ListNewList();
    pipes = ListNewList();
    ready_queue = ListNewList();
    clock_block_procs = ListNewList();

    ttys = (Tty *) calloc(NUM_TERMINALS, sizeof(Tty));
    unsigned int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        TtyInit(&ttys[i]);
    }
}
