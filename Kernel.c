#include "Kernel.h"

#include <assert.h>
#include <stdlib.h>
#include <hardware.h>
#include <string.h>

#include "LoadProgram.h"
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

KernelContext *SaveCurrentKernelContext(KernelContext *kernel_context, void *current_pcb, void *next_pcb);

KernelContext *SaveKernelContextAndSwitch(KernelContext *kernel_context, void *current_pcb, void *next_pcb);

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

    // DEBUG: Print the value at the address of TrapClock().
    unsigned int **trap_table_ptr = (unsigned int **) ReadRegister(REG_VECTOR_BASE);
    unsigned int value_at_addr_of_trap_clock = *(trap_table_ptr[TRAP_CLOCK]);
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Value at address of TrapClock(): %u\n",
            value_at_addr_of_trap_clock);

    // Create the current process
    UserContext model_user_context = *uctxt;
    current_proc = NewBlankPCB(model_user_context);

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
    current_proc->kernel_context_initialized = true;

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

    // DEBUG: Print the value at the address of TrapClock().
    trap_table_ptr = (unsigned int **) ReadRegister(REG_VECTOR_BASE);
    value_at_addr_of_trap_clock = *(trap_table_ptr[TRAP_CLOCK]);
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Value at address of TrapClock(): %u\n",
            value_at_addr_of_trap_clock);

    InitBookkeepingStructs();

    // Load the idle program into the current process.
    int rc = 0;
    rc = LoadProgram("idle", NULL, current_proc);
    if (KILL == rc) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "KernelStart: FAILED TO LOAD IDLE!!\n");
        exit(THEYNIX_EXIT_FAILURE);
    }

    // Create the init process.
    PCB *init_proc = NewBlankPCBWithPageTables(model_user_context, unused_frames);

    
    // Load the init program.
    char *init_program_name = calloc(5, sizeof(char));
    strncpy(init_program_name, "init\0", 5);
    // TODO: do commands correctly
    /*
    if (cmd_args[0]) {
        init_program_name = cmd_args[0];
    }
    */
    WriteRegister(REG_PTBR1, (unsigned int) init_proc->region_1_page_table);
    char **init_args = calloc(2, sizeof(char*));
    init_args[0] = init_program_name;
    rc = LoadProgram(init_program_name, init_args, init_proc);
    WriteRegister(REG_PTBR1, (unsigned int) current_proc->region_1_page_table);
    if (KILL == rc) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "KernelStart: FAILED TO LOAD INIT!!\n");
        exit(THEYNIX_EXIT_FAILURE);
    }
    ListEnqueue(ready_queue, init_proc, init_proc->pid);

    // Run the init proc.
    //current_proc = init_proc;
    //UseKernelStackForProc(current_proc);

    // Use the init proc's user context after returning from KernelStart().
    *uctxt = current_proc->user_context;
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "RESUMED!!!\n\n");
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

void CopyKernelStackPte(PCB *source, PCB *dest) {
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    unsigned int kernel_stack_limit_page = ADDR_TO_PAGE(KERNEL_STACK_LIMIT - 1) + 1;
    unsigned int i;
    for (i = kernel_stack_base_page; i < kernel_stack_limit_page; i++) {
       dest->kernel_stack_page_table[i] = source->kernel_stack_page_table[i];
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

void SaveKernelContext() {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SaveKernelContext()\n");
    int rc = KernelContextSwitch(&SaveCurrentKernelContext, current_proc, NULL);
    if (THEYNIX_EXIT_SUCCESS == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Succesfully saved kernel context!\n");
    } else {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Failed to save kernel context!\n");
    }
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SaveKernelContext()\n");
}

void SwitchToNextProc(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SwitchToNextProc()\n");
    assert(user_context);

    assert(!ListEmpty(ready_queue));
    PCB *next_proc = ListDequeue(ready_queue);
    assert(next_proc);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Loading next proc context into %p\n", user_context);
    *user_context = next_proc->user_context;
    // Set the TLB registers for the region 1 page table.
    WriteRegister(REG_PTBR1, (unsigned int) next_proc->region_1_page_table);

    PCB *old_proc = current_proc;
    current_proc = next_proc;
    int rc = KernelContextSwitch(&SaveKernelContextAndSwitch, old_proc, next_proc);
    if (THEYNIX_EXIT_SUCCESS == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Succesfully switched kernel context!\n");
    } else {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Failed to switch kernel context!\n");
        // TODO: more gracefully handle the failure case
        exit(-1);
    }
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SwitchToNextProc()\n");
}

KernelContext *SaveCurrentKernelContext(KernelContext *kernel_context, void *current_pcb,
        void *next_pcb) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SaveCurrentKernelContext()\n");

    ((PCB*) current_pcb)->kernel_context = *kernel_context;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SaveCurrentKernelContext()\n");
    return kernel_context;
}

KernelContext *SaveKernelContextAndSwitch(KernelContext *kernel_context, void *__current_pcb,
        void *__next_pcb) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SaveKernelContextAndSwitch()\n");
    PCB *current_pcb = (PCB *) __current_pcb;
    PCB *next_pcb = (PCB *) __next_pcb;

    SaveCurrentKernelContext(kernel_context, current_pcb, next_pcb);
    if (!next_pcb->kernel_context_initialized) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Initializing new Kernel Context for proc %p\n", next_pcb);
        next_pcb->kernel_context = current_pcb->kernel_context;
        CopyKernelStackPte(current_pcb, next_pcb);
        next_pcb->kernel_context_initialized = true;
    }

    // Use the new proc's kernel stack page table entries in the region 0 page table.
    UseKernelStackForProc(next_pcb);

    // FLUSH!!!
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SaveKernelContextAndSwitch()\n");
    return &(next_pcb->kernel_context);
}
