#include "VMem.h"

#include <assert.h>
#include <stdlib.h>

#include "Log.h"

/*
 * VMem.c
 * Datastructures and helper methods for managed virtual memory.
 */

extern PCB *current_proc;
extern struct pte *region_0_page_table;

/*
  Mallocs and initializes a region 1 page table with all invalid entries.
*/
void CreateRegion1PageTable(PCB *pcb) {
    pcb->region_1_page_table = (struct pte *) calloc(NUM_PAGES_REG_1, sizeof(struct pte));

    unsigned int i;
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        pcb->region_1_page_table[i].valid = 0;
    }
}

/*
  Starting at the given page number in region 1, maps num_pages pages to newly allocated
  frames. All of the page table entries covered must be invalid prior to this call, and all will be
  valid, with the given protections, after the call. Returns THEYNIX_EXIT_FAILURE if there
  is not enough physical memory available to complete this call.
*/
int MapNewRegion1Pages(PCB *pcb, unsigned int start_page_num,
        unsigned int num_pages, unsigned int prot) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewRegion1Pages()\n");

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(!(pcb->region_1_page_table[page_num].valid));

        // get a new frame for each page
        if (GetUnusedFrame(&pcb->region_1_page_table[page_num]) == ERROR) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "Not enough unused physical frames to complete request.\n");

            // Unmap pages that were mapped to rollback changes made thus far
            UnmapRegion1Pages(pcb, start_page_num, page_num - start_page_num);

            return ERROR;
        }

        // setup pte info
        pcb->region_1_page_table[page_num].prot = prot;
        pcb->region_1_page_table[page_num].valid = 1;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewRegion1Pages()\n\n");
    return SUCCESS;
}

/*
  Starting at the given page number in region 1, unmaps num_pages pages to allocated
  frames. All of the page table entries covered must be valid prior to this call, and all will be
  invalid after the call.
*/
void UnmapRegion1Pages(PCB *pcb, unsigned int start_page_num,
        unsigned int num_pages) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> UnmapNewRegion1Pages()\n");

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(pcb->region_1_page_table[page_num].valid);

        unsigned int pfn = pcb->region_1_page_table[page_num].pfn;
        ReleaseUsedFrame(pfn);

        pcb->region_1_page_table[page_num].valid = 0;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< UnmapNewRegion1Pages()\n\n");
}

/*
  Starting at the given page number in region 1, changes the protections on the next num_pages.
  All of the page table entries covered must be valid prior to this call.
*/
void ChangeProtRegion1Pages(PCB *pcb, unsigned int start_page_num, unsigned int num_pages,
        unsigned int prot) {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> ChangeProtRegion1Pages()\n");

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(pcb->region_1_page_table[page_num].valid);

        pcb->region_1_page_table[page_num].prot = prot;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< ChangeProtRegion1Pages()\n\n");
}

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewRegion0Page(unsigned int page_number) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewFrame0(%u)\n", page_number);

    assert(page_number < VMEM_0_LIMIT / PAGESIZE);

    assert(!region_0_page_table[page_number].valid);

    if (GetUnusedFrame(&region_0_page_table[page_number]) == ERROR) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "GetUnusedFrame() failed.\n");
        return ERROR;
    }

    region_0_page_table[page_number].valid = 1;
    region_0_page_table[page_number].prot = PROT_READ | PROT_WRITE;

    // Flush!
    WriteRegister(REG_TLB_FLUSH, page_number << PAGESHIFT);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewFrame0()\n\n");
    return SUCCESS;
}

/*
  Unmaps a valid region 0 page, freeing the frame the page was mapped to.
*/
void UnmapUsedRegion0Page(unsigned int page_number) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> UnmapUsedFrame0(%u)\n", page_number);

    assert(page_number < VMEM_0_LIMIT / PAGESIZE);
    assert(region_0_page_table[page_number].valid);

    unsigned int used_frame = region_0_page_table[page_number].pfn;
    ReleaseUsedFrame(used_frame);

    region_0_page_table[page_number].valid = 0;
    WriteRegister(REG_TLB_FLUSH, (unsigned int) &region_0_page_table[page_number]);

    // Flush!
    WriteRegister(REG_TLB_FLUSH, page_number << PAGESHIFT);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< UnmapUsedFrame0()\n\n");
}

/*
  Frees all of the physical frames used by the valid region 1 page table entries,
  and marks all region 1 page table entries as invalid.
*/
void FreeRegion1PageTable(PCB *pcb) {
    unsigned int i;
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        if (pcb->region_1_page_table[i].valid) {
            pcb->region_1_page_table[i].valid = 0;

            unsigned int frame_number = pcb->region_1_page_table[i].pfn;
            ReleaseUsedFrame(frame_number);
        }
    }
}

/*
  Frees the physical frames used by the valid region 0 stack page table entries,
  and marks them as invalid.
*/
void FreeRegion0StackPages(PCB *pcb) {
    int i;
    for (i = 0; i < NUM_KERNEL_PAGES; i++) {
        if (pcb->kernel_stack_page_table[i].valid) {
            pcb->kernel_stack_page_table[i].valid = 0;

            unsigned int frame_number = pcb->kernel_stack_page_table[i].pfn;
            ReleaseUsedFrame(frame_number);
        }
    }
}
