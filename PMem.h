#ifndef _PMEM_H
#define _PMEM_H

#include <stdbool.h>

/*
  Code for keeping track of physical memory.
*/

// TODO: Implement cool stack thing we talked about to keep track of unused frames.

typedef bool *UnusedFrames;

/*
  Returns a pointer to a new UnusedFrames object with all of the frames initially unused.
*/
UnusedFrames NewUnusedFrames(unsigned int pmem_size);

/*
  If there is an unused frame available, sets the pfn of the given struct pte * to an unused frame and marks it as read. Then returns SUCCESS.

  Otherwise, returns ERROR.
*/
int GetUnusedFrame(UnusedFrames unused_frames, struct pte *pte_ptr);

/*
  Marks the given unused frame as used.
*/
void MarkFrameAsUsed(UnusedFrames unused_frames, unsigned int frame);

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UnusedFrames unused_frames, unsigned int frame);

#endif
