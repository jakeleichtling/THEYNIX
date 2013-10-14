#include <assert.h>

#include "PMem.h"

/*
  Returns a pointer to a new UnusedFrames object with all of the frames initially unused.
*/
UnusedFrames NewUnusedFrames(unsigned int pmem_size) {
    assert(pmem_size > 0);

    unsigned int num_pages = pmem_size / PAGESIZE;
    UnusedFrames unused_frames = malloc(num_pages * sizeof(bool));

    unsigned int i;
    for (i = 0; i < num_pages; i++) {
        unused_frames[i] = true;
    }
}

/*
  Returns the physical frame number of an unused frame and marks it as used,
  or -1 if there are no frames available.
*/
int GetUnusedFrame(UnusedFrames unused_frames) {
    assert(unused_frames);

    int i;
    for (i = 0; i < num_pages; i++) {
        if (unused_frames[i]) {
            unused_frames[i] = false;
            return i;
        }
    }

    return -1;
}

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UnusedFrames unused_frames, unsigned int frame);
    assert(unused_frames);
    assert(!unused_frames[frame]);

    unused_frames[frame] = true;
}

#endif
