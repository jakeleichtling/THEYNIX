#include "PCB.h"
#include "PMem.h"

#define NUM_PAGES_REG_1 VMEM_1_SIZE / PAGESIZE

/*
  Functions for dealing with virtual memory.
*/

/*
  Mallocs and initializes a region 1 page table with all invalid entries.
*/
void CreateRegion1PageTable(PCB *pcb);

/*
  Frees all of the physical frames used by the valid region 1 page table entries,
  and marks all region 1 page table entries as invalid.
*/
void FreeRegion1PageTable(PCB *pcb, UnusedFrames unused_frames);

/*
  Starting at the given page number in region 1, maps num_pages pages to newly allocated
  frames. All of the page table entries covered must be invalid prior to this call, and all will be
  valid, with the given protections, after the call. Returns THEYNIX_EXIT_FAILURE if there
  is not enough physical memory available to complete this call.
*/
int MapNewRegion1Pages(PCB *pcb, UnusedFrames unused_frames, unsigned int start_page_num,
        unsigned int num_pages, unsigned int prot);

/*
  Starting at the given page number in region 1, unmaps num_pages pages to allocated
  frames. All of the page table entries covered must be valid prior to this call, and all will be
  invalid after the call.
*/
void UnmapRegion1Pages(PCB *pcb, UnusedFrames unused_frames, unsigned int start_page_num,
        unsigned int num_pages);

/*
  Starting at the given page number in region 1, changes the protections on the next num_pages.
  All of the page table entries covered must be valid prior to this call.
*/
void ChangeProtRegion1Pages(PCB *pcb, unsigned int start_page_num, unsigned int num_pages,
        unsigned int prot);

/*
  Retrieves an unused frame, marks it as used, and maps the given region 0 page number
  to the frame. Returns -1 on failure.
*/
int MapNewRegion0Page(unsigned int page_number, UnusedFrames unused_frames);

/*
  Unmaps a valid region 0 page, freeing the frame the page was mapped to.
*/
void UnmapUsedRegion0Page(unsigned int page_number, UnusedFrames unused_frames);
