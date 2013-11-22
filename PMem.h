#ifndef _PMEM_H
#define _PMEM_H

#include <stdbool.h>
#include <hardware.h>

/*
  Code for keeping track of physical memory.
*/

/*
  Initializes the data structure for keeping track of physical memory:

  There is a linked list that contains all free frames. Each frame, at offset 0, contains an int
  that is the number of the next free frame. Each free frame, at offset 1, contains an int pointing
  to the previous free frame. There is a global variable, free_frames_head, that
  contains the number of the first free frame in the linked list, and a global variable, free_frames_tail,
  that contains the last free frame in the linked list.
*/
void InitializePhysicalMemoryManagement();

/*
  If there is an unused frame available, sets the pfn of the given struct pte * to an unused frame
  and marks it as used. Then returns SUCCESS.

  Otherwise, returns ERROR.
*/
int GetUnusedFrame(struct pte *pte_ptr);

/*
  Marks the given unused frame as used.
*/
void MarkFrameAsUsed(int frame);

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(int frame);

#endif
