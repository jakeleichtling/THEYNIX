#include <assert.h>

#include "PMem.h"

/*
  Returns a pointer to a new UnusedFrames object with all of the frames initially unused.
*/
UnusedFrames NewUnusedFrames(unsigned int pmem_size) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> NewUnusedFrames(pmem_size=%u)\n",
            pmem_size);

    assert(pmem_size > 0);

    unsigned int num_pages = pmem_size / PAGESIZE;
    UnusedFrames unused_frames = malloc(num_pages * sizeof(bool));
    assert(unused_frames);

    unsigned int i;
    for (i = 0; i < num_pages; i++) {
        unused_frames[i] = true;
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< NewUnusedFrames()\n\n");
}

/*
  Returns the physical frame number of an unused frame and marks it as used,
  or -1 if there are no frames available.
*/
int GetUnusedFrame(UnusedFrames unused_frames) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> GetUnusedFrame()\n");

    assert(unused_frames);

    int i;
    for (i = 0; i < num_pages; i++) {
        if (unused_frames[i]) {
            unused_frames[i] = false;
            TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< GetUnusedFrame() --> %d\n\n", i);
            return i;
        }
    }

    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No unused frame.\n");
    return EXIT_FAILURE;
}

/*
  Marks the given unused frame as used.
*/
int MarkFrameAsUsed(UnusedFrames unused_frames, unsigned int frame) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> MarkFrameAsUsed(frame=%u)\n", frame);

    assert(unused_frames);
    assert(unused_frames[frame]);

    unused_frames[frame] = false;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< MarkFrameAsUsed()\n\n");
}

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UnusedFrames unused_frames, unsigned int frame) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> ReleaseUsedFrame(frame=%u", frame);

    assert(unused_frames);
    assert(!unused_frames[frame]);

    unused_frames[frame] = true;

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< ReleaseUsedFrame()\n\n");
}

#endif
