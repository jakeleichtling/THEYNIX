#ifndef _PMEM_H
#define _PMEM_H

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
  Returns the physical frame number of an unused frame and marks it as used,
  or -1 if there are no frames available.
*/
int GetUnusedFrame(UnusedFrames unused_frames);

/*
  Marks the given frame as used. Returns -1 if the frame was not previously unused.
*/
int MarkFrameAsUsed(UnusedFrames unused_frames, unsigned int frame);

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UnusedFrames unused_frames, unsigned int frame);

#endif
