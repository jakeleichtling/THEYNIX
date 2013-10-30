#include "VMem.h"

#include <assert.h>
#include <stdlib.h>

#include "Log.h"

extern PCB *current_proc;
extern struct pte *region_0_page_table;
extern UnusedFrames unused_frames;

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
}

/*
  Starting at the given page number in region 1, maps num_pages pages to newly allocated
  frames. All of the page table entries covered must be invalid prior to this call, and all will be
  valid, with the given protections, after the call. Returns THEYNIX_EXIT_FAILURE if there
  is not enough physical memory available to complete this call.
*/
int MapNewRegion1Pages(PCB *pcb, UnusedFrames unused_frames, unsigned int start_page_num,
        unsigned int num_pages, unsigned int prot) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewRegion1Pages()\n");

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(!(pcb->region_1_page_table[page_num].valid));

        pcb->region_1_page_table[page_num].pfn = GetUnusedFrame(unused_frames);
        if (pcb->region_1_page_table[page_num].pfn == THEYNIX_EXIT_FAILURE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "Not enough unused physical frames to complete request.\n");
            return THEYNIX_EXIT_FAILURE;
        }

        pcb->region_1_page_table[page_num].prot = prot;
        pcb->region_1_page_table[page_num].valid = 1;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewRegion1Pages()\n\n");
    return THEYNIX_EXIT_SUCCESS;
}

/*
  Starting at the given page number in region 1, unmaps num_pages pages to allocated
  frames. All of the page table entries covered must be valid prior to this call, and all will be
  invalid after the call.
*/
void UnmapRegion1Pages(PCB *pcb, UnusedFrames unused_frames, unsigned int start_page_num,
        unsigned int num_pages) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> UnmapNewRegion1Pages()\n");

    unsigned int page_num;
    for (page_num = start_page_num; page_num < start_page_num + num_pages; page_num++) {
        assert(page_num < NUM_PAGES_REG_1);
        assert(pcb->region_1_page_table[page_num].valid);

        unsigned int pfn = pcb->region_1_page_table[page_num].pfn;
        ReleaseUsedFrame(unused_frames, pfn);

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
int MapNewRegion0Page(unsigned int page_number, UnusedFrames unused_frames) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MapNewFrame0(%u)\n", page_number);

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

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MapNewFrame0()\n\n");
    return THEYNIX_EXIT_SUCCESS;
}

/*
  Unmaps a valid region 0 page, freeing the frame the page was mapped to.
*/
void UnmapUsedRegion0Page(unsigned int page_number, UnusedFrames unused_frames) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> UnmapUsedFrame0(%u)\n", page_number);

    assert(page_number < VMEM_0_LIMIT / PAGESIZE);
    assert(region_0_page_table[page_number].valid);

    unsigned int used_frame = region_0_page_table[page_number].pfn;
    ReleaseUsedFrame(unused_frames, used_frame);

    region_0_page_table[page_number].valid = 0;
    WriteRegister(REG_TLB_FLUSH, (unsigned int) &region_0_page_table[page_number]);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< UnmapUsedFrame0()\n\n");
}

/*
  For each valid page in the current process's region 1 page table, releases the corresponding
  frame and marks the page as invalid.
*/
void ReleaseAllRegion1ForCurrentProc() {
    int i;
    for (i = 0; i < NUM_PAGES_REG_1; i++) {
        if (current_proc->region_1_page_table[i].valid) {
            ReleaseUsedFrame(unused_frames, current_proc->region_1_page_table[i].pfn);
            current_proc->region_1_page_table.valid = 0;
        }
    }
}
