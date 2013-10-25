#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <stdbool.h>

#include "include/hardware.h"
#include "List.h"
#include "PCB.h"
#include "PMem.h"
#include "Tty.h"

/* Macros */

#define ADDR_TO_PAGE(addr) (((unsigned int) addr) >> PAGESHIFT)

#define KILL -1337

/* Global Variables */

List *locks;
List *cvars;
List *pipes;

Tty *ttys;

PCB *current_proc;
List *ready_queue;
List *clock_block_procs;

UnusedFrames unused_frames;
bool virtual_memory_enabled;

// The lowest page number not in use by the kernel's data segment. Starting at
// kernel_data_start_page and covering up to, but not including, this page should have
// PROT_READ | PROT_WRITE permissions.
unsigned int kernel_brk_page;
// The lowest page number used by the kernel's data segment. The pages up to,
// but not including, this page is the text, and thus should have
// PROT_READ | PROT_EXEC permissions.
unsigned int kernel_data_start_page;

struct pte *region_0_page_table;

/* Function Prototypes */

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd);

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

int SetKernelBrk(void *addr);

// Get a copy of the currently running Kernel Context and save it in the current pcb
void SaveKernelContext();

// Context switch to the next process in the ready queue.
// The next process's context will be loaded into the param user_context.
// NOTE: place the current proc into the correct queue before calling
// (e.g., ready queue, clock blocked queue)
void SwitchToNextProc(UserContext *user_context);

#endif
