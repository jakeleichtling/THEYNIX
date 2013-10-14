/* Global Variables */

List *locks;
List *cvars;
List *pipes;
Tty *ttys;

PCB *current_proc;
List *ready_queue;
List *clock_block_procs;

UsedFrames used_frames;

int kernel_brk_page;
int kernel_data_start_page;

/* Function Prototypes */

void SetKernelData(void *_KernelDataStart, void *_KernelDataEnd);

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

int SetKernelBrk(void *addr);
