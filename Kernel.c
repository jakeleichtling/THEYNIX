#include "Kernel.h"

#include <assert.h>
#include <stdlib.h>
#include <hardware.h>

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

/*
  Copies the data in the region 0 source page number to the frame mapped by the region 0 dest page number.
*/
void CopyRegion0PageData(unsigned int source_page_number, unsigned int dest_page_number);

/*
  Copies the data in the region 1 source page number to the frame mapped by the region 1 dest page number.
*/
void CopyRegion1PageData(unsigned int source_page_number, unsigned int dest_page_number);

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

    // Create the init proc
    UserContext model_user_context = *uctxt;
    PCB *init_proc = NewBlankPCB(model_user_context);

    // Perform the malloc for the init proc's kernel stack page table before making page tables.
    init_proc->kernel_stack_page_table =
            (struct pte *) calloc(KERNEL_STACK_MAXSIZE / PAGESIZE, sizeof(struct pte));

    // Build the initial page table for region 0 such that page = frame for all valid pages.
    region_0_page_table = (struct pte *) calloc(VMEM_0_SIZE / PAGESIZE, sizeof(struct pte));

    // Create the init proc's page table for region 1.
    CreateRegion1PageTable(init_proc);

    // Create the PTEs for the kernel text and data with the proper protections.
    unsigned int i;
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

    // Create the PTEs for the init proc's kernel stack with page = frame and the proper protections.
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    for (i = 0; i < NUM_KERNEL_PAGES; i++) {
        init_proc->kernel_stack_page_table[i].valid = 1;
        init_proc->kernel_stack_page_table[i].pfn = i + kernel_stack_base_page;
        MarkFrameAsUsed(unused_frames, i + kernel_stack_base_page);

        init_proc->kernel_stack_page_table[i].prot = PROT_READ | PROT_WRITE;
    }
    UseKernelStackForProc(init_proc);
    init_proc->kernel_context_initialized = true;

    // Set the TLB registers for the region 0 page table.
    WriteRegister(REG_PTBR0, (unsigned int) region_0_page_table);
    WriteRegister(REG_PTLR0, VMEM_0_SIZE / PAGESIZE);

    // Set the TLB registers for the region 1 page table.
    WriteRegister(REG_PTBR1, (unsigned int) init_proc->region_1_page_table);
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

    // Load the init program.
    char *init_program_name = "init";
    if (cmd_args[0]) {
        init_program_name = cmd_args[0];
    }
    int rc = LoadProgram(init_program_name, cmd_args, init_proc);
    if (rc != SUCCESS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "KernelStart: FAILED TO LOAD INIT!!\n");
        exit(THEYNIX_EXIT_FAILURE);
    }

    // Load the idle program, but first make sure we are pointing to its region 1 page table.
    PCB *idle_proc = NewBlankPCBWithPageTables(model_user_context, unused_frames);
    WriteRegister(REG_PTBR1, (unsigned int) idle_proc->region_1_page_table);
    rc = LoadProgram("idle", NULL, idle_proc);
    if (rc != SUCCESS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "KernelStart: FAILED TO LOAD IDLE!!\n");
        exit(THEYNIX_EXIT_FAILURE);
    }

    // Put idle in the ready queue.
    ListEnqueue(ready_queue, idle_proc, idle_proc->pid);

    // Make init the current proc.
    current_proc = init_proc;
    WriteRegister(REG_PTBR1, (unsigned int) init_proc->region_1_page_table);

    // Use the init proc's user context after returning from KernelStart().
    *uctxt = init_proc->user_context;
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
    unsigned int i;
    for (i = 0; i < NUM_KERNEL_PAGES; i++) {
        region_0_page_table[kernel_stack_base_page + i] = pcb->kernel_stack_page_table[i];
    }
}

/*
  Note: This must be executed in the magic kernel context switch space!!!

  First, maps kernel_stack[0] = dest_kernel_stack[-1] and copies
  kernel_stack[0] <-- kernel_stack[-1] = source_kernel_stack[-1].

  Then, maps kernel_stack[0] = source_kernel_stack[0].

  Then, for i = -2 to 0, maps kernel_stack[i+1] = dest_kernel_stack[i] and copies
  kernel_stack[i+1] <-- kernel_stack[i] = source_kernel_stack[i].
*/
void CopyKernelStackPageTableAndData(PCB *source, PCB *dest) {
    unsigned int kernel_stack_base_page = ADDR_TO_PAGE(KERNEL_STACK_BASE);
    int i;

    // First, map kernel_stack[0] = dest_kernel_stack[-1] and copy
    // kernel_stack[0] <-- kernel_stack[-1] = source_kernel_stack[-1].
    region_0_page_table[kernel_stack_base_page] = dest->kernel_stack_page_table[NUM_KERNEL_PAGES - 1];
    WriteRegister(REG_TLB_FLUSH, kernel_stack_base_page << PAGESHIFT);
    CopyRegion0PageData(kernel_stack_base_page + NUM_KERNEL_PAGES - 1, kernel_stack_base_page);

    // Then, map kernel_stack[0] = source_kernel_stack[0].
    region_0_page_table[kernel_stack_base_page] = source->kernel_stack_page_table[0];
    WriteRegister(REG_TLB_FLUSH, kernel_stack_base_page << PAGESHIFT);

    // Then, for i = -2 to 0, maps kernel_stack[i+1] = dest_kernel_stack[i] and copies
    // kernel_stack[i+1] <-- kernel_stack[i] = source_kernel_stack[i].
    for (i = NUM_KERNEL_PAGES - 2; i >= 0; i--) {
        region_0_page_table[kernel_stack_base_page + i + 1] = dest->kernel_stack_page_table[i];
        WriteRegister(REG_TLB_FLUSH, (kernel_stack_base_page + i + 1) << PAGESHIFT);
        CopyRegion0PageData(kernel_stack_base_page + i, kernel_stack_base_page + i + 1);
    }
}

/*
  For each valid page in the source_region_1 table, allocates a frame in the dest_region_1
  table with PROT_WRITE permission.

  Creates a temporary region 1 page table in the kernel heap and points the TLB to it.

  Maps region_1[0] = dest_region_1[-1] and, if region_1[-1] is valid, copies
  region_1[0] <-- region_1[-1] = source_region_1[-1].

  Maps region_1[0] = source_kernel_stack[0].

  For i = -2 to 0, maps region_1[i+1] = dest_region_1[i]. If region_1[i] is valid, copies
  region_1[i+1] <-- region_1[i] = source_region_1[i].

  For each valid page table entry in the source_region_1 table, sets the corresponding
  dest_region_1 page table entry to have the same protections.
*/
void CopyRegion1PageTableAndData(PCB *source, PCB *dest) { // make sure dest has a region 1 page table calloced
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> CopyRegion1PageTableAndData()\n");

    int i; // Must not be unsigned!

    // For each valid page in the source_region_1 table, allocate a frame in the dest_region_1
    // table with PROT_WRITE permission.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 1\n");
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        if (source->region_1_page_table[i].valid) {
            dest->region_1_page_table[i].valid = 1;
            dest->region_1_page_table[i].prot = PROT_WRITE;
            dest->region_1_page_table[i].pfn = GetUnusedFrame(unused_frames);
        }
    }

    // Create a temporary region 1 page table in the kernel heap that starts out as a copy of the
    // source region 1 page table. Point the TLB to it and flush.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 2\n");
    struct pte *temp_region_1_page_table = (struct pte *) calloc(NUM_PAGES_REG_1, sizeof(struct pte));
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        temp_region_1_page_table[i] = source->region_1_page_table[i];
    }
    WriteRegister(REG_PTBR1, (unsigned int) temp_region_1_page_table);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    // Map region_1[0] = dest_region_1[-1] and, if region_1[-1] is valid, copy
    // region_1[0] <-- region_1[-1] = source_region_1[-1].
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 3\n");
    if (temp_region_1_page_table[NUM_PAGES_REG_1 - 1].valid) {
        temp_region_1_page_table[0] = dest->region_1_page_table[NUM_PAGES_REG_1 - 1];
        WriteRegister(REG_TLB_FLUSH, (VMEM_1_BASE + 0) << PAGESHIFT);
        CopyRegion1PageData(NUM_PAGES_REG_1 - 1, 0);
    }

    // Map region_1[0] = source_kernel_stack[0].
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 4\n");
    temp_region_1_page_table[0] = source->region_1_page_table[0];
    WriteRegister(REG_TLB_FLUSH, (VMEM_1_BASE + 0) << PAGESHIFT);

    // For i = -2 to 0, maps region_1[i+1] = dest_region_1[i]. If region_1[i] is valid, copies
    // region_1[i+1] <-- region_1[i] = source_region_1[i].
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 5\n");
    for (i = NUM_PAGES_REG_1 - 2; i >= 0; i--) {
        if (temp_region_1_page_table[i].valid) {
            temp_region_1_page_table[i + 1] = dest->region_1_page_table[i];
            WriteRegister(REG_TLB_FLUSH, (VMEM_1_BASE + i + 1) << PAGESHIFT);
            CopyRegion1PageData(i, i + 1);
        }
    }

    // For each valid page table entry in the source_region_1 table, sets the corresponding
    // dest_region_1 page table entry to have the same protections.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 6\n");
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        if (source->region_1_page_table[i].valid) {
            dest->region_1_page_table[i].prot = source->region_1_page_table[i].prot;
        }
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< CopyRegion1PageTableAndData()\n\n");
}

/*
  Copies the data in the source page number to the frame mapped by the dest page number.
*/
void CopyRegion0PageData(unsigned int source_page_number, unsigned int dest_page_number) {
    char *source_byte_addr;
    char *dest_byte_addr;
    unsigned int i;
    for (i = 0; i < PAGESIZE; i++) {
        source_byte_addr = (char *)((source_page_number << PAGESHIFT) + i);
        dest_byte_addr = (char *)((dest_page_number << PAGESHIFT) + i);

        *dest_byte_addr = *source_byte_addr;
    }
}

/*
  Copies the data in the source page number to the frame mapped by the dest page number.
*/
void CopyRegion1PageData(unsigned int source_page_number, unsigned int dest_page_number) {
    char *source_byte_addr;
    char *dest_byte_addr;
    unsigned int i;
    for (i = 0; i < PAGESIZE; i++) {
        source_byte_addr = (char *)((source_page_number << PAGESHIFT) + i + VMEM_1_BASE);
        dest_byte_addr = (char *)((dest_page_number << PAGESHIFT) + i + VMEM_1_BASE);

        *dest_byte_addr = *source_byte_addr;
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
    assert(!ListEmpty(ready_queue));
    PCB *next_proc = ListDequeue(ready_queue);

    SwitchToProc(next_proc, user_context);
}

void SwitchToProc(PCB *next_proc, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> SwitchToProc()\n");
    assert(user_context);
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
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SwitchToProc()\n");
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
        CopyKernelStackPageTableAndData(current_pcb, next_pcb);
        next_pcb->kernel_context_initialized = true;
    }

    // Use the new proc's kernel stack page table entries in the region 0 page table.
    UseKernelStackForProc(next_pcb);

    // FLUSH EVERYTHING!!!
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< SaveKernelContextAndSwitch()\n");
    return &(next_pcb->kernel_context);
}
