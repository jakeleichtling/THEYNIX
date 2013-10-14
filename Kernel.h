#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <stdbool.h>

#include "include/hardware.h"

/* Global Variables */

List *locks;
List *cvars;
List *pipes;
Tty *ttys;

PCB *current_proc;
List *ready_queue;
List *clock_block_procs;

UsedFrames used_frames;

bool virtual_memory_enabled = false;

// The lowest page number not in use by the kernel's data segment. Starting at
// kernel_data_start_page and covering up to, but not including, this page should have
// PROT_READ | PROT_WRITE permissions.
int kernel_brk_page;
// The lowest page number used by the kernel's data segment. The pages covering up to,
// but not including, this page should have PROT_READ | PROT_EXEC permissions.
int kernel_data_start_page;

/* Function Prototypes */

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd);

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

int SetKernelBrk(void *addr);

#endif
