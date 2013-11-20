#include "PMem.h"

#include <stdlib.h>
#include <assert.h>

#include "include/hardware.h"
#include "Log.h"

unsigned int num_pages;
unsigned int num_unused_frames;
/*
  Returns a pointer to a new UnusedFrames object with all of the frames initially unused.
*/
UnusedFrames NewUnusedFrames(unsigned int pmem_size) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> NewUnusedFrames(pmem_size=%u)\n",
            pmem_size);

    assert(pmem_size > 0);

    num_pages = pmem_size / PAGESIZE;
    num_unused_frames = num_pages;
    UnusedFrames unused_frames = (UnusedFrames) calloc(num_pages, sizeof(bool));
    assert(unused_frames);
    unsigned int i;
    for (i = 0; i < num_pages; i++) {
        unused_frames[i] = true;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< NewUnusedFrames() [num_unused_frames = %d] \n\n", num_unused_frames);
    return unused_frames;
}

/*
  If there is an unused frame available, sets the pfn of the given struct pte * to an unused frame and marks it as read. Then returns SUCCESS.

  Otherwise, returns ERROR.
*/
int GetUnusedFrame(UnusedFrames unused_frames, struct pte *pte_ptr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> GetUnusedFrame()\n");

    unsigned int i;
    for (i = 0; i < num_pages; i++) {
        if (unused_frames[i]) {
            unused_frames[i] = false;
            num_unused_frames--;
            TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< GetUnusedFrame() --> %d [num_unused_frames = %d]\n\n", i, num_unused_frames);

            pte_ptr->pfn = i;

            return SUCCESS;
        }
    }

    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No unused frame.\n");
    return ERROR;
}

/*
  Marks the given unused frame as used.
*/
void MarkFrameAsUsed(UnusedFrames unused_frames, unsigned int frame) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MarkFrameAsUsed(frame=%u)\n", frame);

    assert(unused_frames);
    assert(unused_frames[frame]);

    unused_frames[frame] = false;
    num_unused_frames--;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MarkFrameAsUsed() [num_unused_frames = %d]\n\n", num_unused_frames);
}

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UnusedFrames unused_frames, unsigned int frame) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> ReleaseUsedFrame(frame=%u)\n", frame);

    assert(unused_frames);
    assert(!unused_frames[frame]);

    unused_frames[frame] = true;
    num_unused_frames++;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< ReleaseUsedFrame() [num_unused_frames = %d]\n\n", num_unused_frames);
}
