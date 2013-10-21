#include "VMem.h"

#include <stdlib.h>

/*
  Mallocs and initializes a region 1 page table with all invalid entries.
*/
void CreateRegion1PageTable(PCB *pcb) {
    pcb->region_1_page_table = (struct pte *) malloc(NUM_PAGES_REG_1 * sizeof(struct pte));

    unsigned int i;
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        pcb->region_1_page_table[i].valid = 0;
    }
}

/*
  Frees all of the physical frames used by the valid region 1 page table entries,
  and marks all region 1 page table entries as invalid.
*/
void FreeRegion1PageTable(PCB *pcb, UnusedFrames unused_frames) {
    unsigned int i;
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        if (pcb->region_1_page_table[i].valid) {
            pcb->region_1_page_table[i].valid = 0;

            unsigned int frame_number = pcb->region_1_page_table[i].pfn;
            ReleaseUsedFrame(unused_frames, frame_number);
    }
}

/*
  Starting at the given page number in region 1, maps num_pages pages to newly allocated
  frames. All of the page table entries covered must be invalid prior to this call, and all will be
  valid, with the given protections, after the call. Returns THEYNIX_EXIT_FAILURE if there
  is not enough physical memory available to complete this call.
*/
int MapNewRegion1Pages(PCB *pcb, UnusedFrames unused_frames, unsigned int start_page_num,
        unsigned int num_pages, unsigned int prot) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewRegion1Pages()\n")

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(!(pcb->region_1_page_table[page_num].valid));

        pcb->region_1_page_table.pfn = GetUnusedFrame(unused_frames);
        if (pcb->region_1_page_table.pfn == THEYNIX_EXIT_FAILURE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "Not enough unused physical frames to complete request.\n");
            return THEYNIX_EXIT_FAILURE;
        }

        pcb->region_1_page_table.prot = prot;
        pcb->region_1_page_table.valid = 1;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewRegion1Pages()\n\n")
}

/*
  Starting at the given page number in region 1, changes the protections on the next num_pages.
  All of the page table entries covered must be valid prior to this call.
*/
void ChangeProtRegion1Pages(PCB *pcb, unsigned int start_page_num, unsigned int num_pages,
        unsigned int prot) {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> ChangeProtRegion1Pages()\n")

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(pcb->region_1_page_table[page_num].valid);

        pcb->region_1_page_table.prot = prot;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< ChangeProtRegion1Pages()\n\n")
}
