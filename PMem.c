/*
  Function implementations and static variables for keeping track of physical memory.
*/

/*
  There is a linked list that contains all free frames. Each frame, at offset 0, contains an int
  that is the number of the next free frame. Each free frame, at offset 1, contains an int pointing
  to the previous free frame. There is a global variable, free_frames_head, that
  contains the number of the first free frame in the linked list, and a global variable, free_frames_tail,
  that contains the last free frame in the linked list.
*/

#include "PMem.h"

#include <stdlib.h>

#include "Kernel.h"
#include "Log.h"
#include "PCB.h"

int free_frames_head; // -1 if list is empty.
int free_frames_tail; // -1 if list is empty.

// From Kernel.h
extern PCB *current_proc;
extern unsigned int kernel_brk_page;

/*    Private Function Prototypes     */
void AddToLinkedList(int frame_number);
int GetNextFreeFrameNumber(int frame_number);
void SetNextAndPrevFreeFrameNumbers(int frame_a, int next_frame_b, int prev_frame_c);

/*
  Initialize the data structures for keeping track of physical memory.
*/
void InitializePhysicalMemoryManagement(unsigned int pmem_size) {
  int max_frame = ((pmem_size + PMEM_BASE) >> PAGESHIFT) - 1;

  // Set head and tail to first frame above kernel heap, which is the kernel brk.
  free_frames_head = free_frames_tail = kernel_brk_page;

  // Starting with free_frames_head and up to, but not including, the bottom of the kernel stack,
  // add frames to the free frames list.
  int i;
  for (i = free_frames_head + 1; i < ADDR_TO_PAGE(KERNEL_STACK_BASE); i++) {
    AddToLinkedList(i);
  }

  // Starting with the first frame above the kernel stack and up to, including, the max_frame,
  // add frames to the free frames list.
  for (i = ADDR_TO_PAGE(KERNEL_STACK_LIMIT) + 1; i <= max_frame; i++) {
    AddToLinkedList(i);
  }
}

/*
  If there is an unused frame available, sets the pfn of the given struct pte * to an unused frame
  and marks it as used. Then returns SUCCESS.

  Otherwise, returns ERROR.
*/
int GetUnusedFrame(struct pte *pte_ptr) {
  // Return error if the list is empty.
  if (free_frames_head < 0) {
    return ERROR;
  }

  // Get the first frame in the list and store it in pte_ptr->pfn.
  pte_ptr->pfn = free_frames_head;

  // If free_frames_head was the tail, set both head and tail to -1 to indicate that we are out
  // of free frames.
  if (free_frames_head == free_frames_tail) {
    free_frames_head = free_frames_tail = -1;
    return SUCCESS;
  }

  // Otherwise, set free_frames_head to the next frame in the list.
  int next_frame_number = GetNextFreeFrameNumber(free_frames_head);
  free_frames_head = next_frame_number;

  return SUCCESS;
}

/*
  Marks the given used frame as unused.
*/
void ReleaseUsedFrame(int frame_number) {
  AddToLinkedList(frame_number);
}

/*
  Given the frame number of a free frame A and free frame numbers B and C, sets A to point to B
  as the next free frame and C as the previous free frame in the linked list.

  If A was the tail, the tail is now B. If A was head, the head is now C.

  If either B or C is set to -1, then next or pev is not set, respectively.
*/
void SetNextAndPrevFreeFrameNumbers(int frame_a, int next_frame_b, int prev_frame_c) {
  bool a_was_tail = (frame_a == free_frames_tail);
  bool a_was_head = (frame_a == free_frames_head);

  // Map frame A into the first page of region 1.
  int actual_pfn = current_proc->region_1_page_table[0].pfn;
  current_proc->region_1_page_table[0].pfn = frame_a;
  WriteRegister(REG_TLB_FLUSH, VMEM_1_BASE);

  // Write next_frame_b into A[0].
  int *frame_ptrs = (int *) VMEM_1_BASE;
  if (next_frame_b >= 0) {
    frame_ptrs[0] = next_frame_b;

    if (a_was_tail) {
      free_frames_tail = next_frame_b;
    }
  }

  // Write prev_frame_c into A[1].
  if (prev_frame_c >= 0) {
    frame_ptrs[1] = prev_frame_c;

    if (a_was_head) {
      free_frames_head = prev_frame_c;
    }
  }

  // Remap the first page of region 1.
  current_proc->region_1_page_table[0].pfn = actual_pfn;
  WriteRegister(REG_TLB_FLISH, VMEM_1_BASE);
}

/*
  Appends the given frame to the linked list, making it the new tail.
*/
void AddToLinkedList(int frame_number) {
  int prev_tail = free_frames_tail;

  SetNextAndPrevFreeFrameNumbers(free_frames_tail, frame_number, -1);
  SetNextAndPrevFreeFrameNumbers(frame_number, -1, prev_tail);
}

/*
  Given the frame number of a free frame, returns the free frame number of the next frame
  in the free frames linked list.

  Returns -1 if there is no next frame.
*/
int GetNextFreeFrameNumber(int frame_number) {
  if (frame_number == free_frames_tail) {
    return -1;
  }

  // Map the frame into the first page of region 1.
  int actual_pfn = current_proc->region_1_page_table[0].pfn;
  current_proc->region_1_page_table[0].pfn = frame_number;
  WriteRegister(REG_TLB_FLISH, VMEM_1_BASE);

  // Obtain the next free frame number at frame[0];
  int *frame_ptrs = (int *) VMEM_1_BASE;
  int next_frame_number = frame_ptrs[0];

  // Remap the first page of region 1.
  current_proc->region_1_page_table[0].pfn = actual_pfn;
  WriteRegister(REG_TLB_FLISH, VMEM_1_BASE);

  // Return the next free frame number.
  return next_frame_number;
}
