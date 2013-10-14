#ifndef _PMEM_H
#define _PMEM_H

/*
  Code for keeping track of physical memory.
*/

// TODO: Implement cool stack thing we talked about to keep track of used frames.

typedef bool *UsedFrames;

/*
  Returns the physical frame number of an unused frame and marks it as used.
*/
int GetUnusedFrame(UsedFrames used_frames);

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(UsedFrames used_frames, int frame);

#endif
