#include "SystemCalls.h"

#include <stdlib.h>
#include <string.h>

#include "LoadProgram.h"
#include "Log.h"
#include "Kernel.h"
#include "PMem.h"
#include "VMem.h"

extern UnusedFrames unused_frames;
extern List *clock_block_procs;

/*
  Implementations of Yalnix system calls.
*/

int KernelFork(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelFork()\n");

    // Save the current user context.
    current_proc->user_context = *user_context;

    // Make a new child PCB with the same user context as the parent.
    PCB *child_pcb = NewBlankPCBWithPageTables(current_proc->user_context, unused_frames);
    child_pcb->waiting_on_children = false;
    child_pcb->lowest_user_stack_page = current_proc->lowest_user_stack_page;
    child_pcb->user_brk_page = current_proc->user_brk_page;

    // Copy over region 1.
    CopyRegion1PageTableAndData(current_proc, child_pcb);

    // Add the child to the parent's child list
    ListEnqueue(current_proc->live_children, child_pcb, child_pcb->pid);

    // Set child's parent pointer
    child_pcb->live_parent = current_proc;

    // Add the child to the ready queue
    ListEnqueue(ready_queue, child_pcb, child_pcb->pid);

    // Record the child's PID for later comparison.
    unsigned int child_pid = child_pcb->pid;

    // Set kernel_context_initialized to false and context switch to
    // child so that the KernelContext and kernel stack are copied from parent.
    ListEnqueue(ready_queue, current_proc, current_proc->pid);
    child_pcb->kernel_context_initialized = false;
    SwitchToProc(child_pcb, user_context);

    // Compare the current PID to the child's PID to return correct value.
    if (child_pid == current_proc->pid) {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelFork() [child: pid = %d] \n\n", current_proc->pid);
        return 0;
    } else {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelFork() [parent: pid = %d] \n\n", current_proc->pid);
        return child_pid;
    }

    return 0;
}

int KernelExec(char *filename, char **argvec) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelExec()\n");

    // Copy the filename string and arguments to the Kernel heap.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 1\n");
    int filename_len = strlen(filename);
    char *heap_filename = calloc(filename_len + 1, sizeof(char));
    strncpy(heap_filename, filename, filename_len);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 2\n");
    char **heap_argvec = NULL;
    if (argvec) {
        int num_args = sizeof(argvec) / sizeof(char *);
        char **heap_argvec = calloc(num_args + 1, sizeof(char *));
        int i;
        for (i = 0; i < num_args; i++) {
            char *arg = argvec[i];
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Copying arg %d: %s\n", i, arg);

            int arg_len = strlen(arg);
            char *heap_arg = calloc(arg_len + 1, sizeof(char));
            strncpy(heap_arg, arg, arg_len);

            heap_argvec[i] = heap_arg;
        }
    }

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 3\n");
    // Create the new region 1 page table, loading the executable text from the given file.
    // LoadProgram() also frees the entire region 1 before recreating it for the new program.
    current_proc->user_context.sp = 0;
    LoadProgram(heap_filename, heap_argvec, current_proc);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 4\n");
    // Free the filename string and arguments in the Kernel heap.
    free(heap_filename);
    if (argvec) {
        int num_args = sizeof(argvec) / sizeof(char *);
        int i;
        for (i = 0; i < num_args; i++) {
            char *heap_arg = heap_argvec[i];
            free(heap_arg);
        }
        free(heap_argvec);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelExec()\n\n");

    // Return
    return THEYNIX_EXIT_SUCCESS;
}

void KernelExit(int status) {
    // If initial process, halt system

    // Set parent pointers of children to null

    // Save exit status

    // Free all frames

    // If has a parent, move proc to zombie_children list of parent

    // If parent is waiting_on_children, move parent proc to ready queue from blocked, and reset waiting_on_chilrden

    // If doesn't have parent, free PCB

    // Context switch (do we free kernel stack frames here?)
}

int KernelWait(int *status_ptr) {
    // If zombie children list is not empty, collect exit status of one, remove PCB from list, and free

    // If zombie children list is empty
        // If no live children, return error
        // Else, indicate waiting_for_children, move to blocked, context switch

    // Return the exit status
    return THEYNIX_EXIT_SUCCESS;
}

int KernelGetPid(void) {
    return current_proc->pid;
}

int KernelBrk(void *addr) {
    // TODO: verify memory
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelBrk(%p)\n", addr);

    unsigned int new_user_brk_page = ADDR_TO_PAGE(addr - 1) + 1;

    // Ensure we aren't imposing on user stack limits.
    if (new_user_brk_page >= current_proc->lowest_user_stack_page) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Address passed to KernelBrk() (%p) does not leave a blank page between heap and user stack page (%d).\n",
                new_user_brk_page, current_proc->lowest_user_stack_page);
        return THEYNIX_EXIT_FAILURE;
    }

    if (new_user_brk_page > current_proc->user_brk_page) {
        int rc = MapNewRegion1Pages(current_proc, unused_frames, current_proc->user_brk_page,
                new_user_brk_page - current_proc->user_brk_page, PROT_READ | PROT_WRITE);
        if (rc == THEYNIX_EXIT_FAILURE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "MapNewRegion1Pages() failed.\n");
            return THEYNIX_EXIT_FAILURE;
        }
    } else if (new_user_brk_page < current_proc->user_brk_page) {
        UnmapRegion1Pages(current_proc, unused_frames, new_user_brk_page,
                current_proc->user_brk_page - new_user_brk_page);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelBrk()\n\n");
    current_proc->user_brk_page = new_user_brk_page;
    return 0;
}

int KernelDelay(int clock_ticks, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelDelay(ticks=%d)\n", clock_ticks);
    // If clock_ticks < 0, return error
    if (clock_ticks < 0) {
        return THEYNIX_EXIT_FAILURE;
    }

    // If clock_ticks == 0, return success
    if (0 == clock_ticks) {
        return THEYNIX_EXIT_SUCCESS;
    }

    // Set clock ticks remaining to clock_ticks
    current_proc->clock_ticks_until_ready = clock_ticks;

    // Put proc in list of clock blocked
    ListEnqueue(clock_block_procs, current_proc, current_proc->pid);

    SwitchToNextProc(user_context);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelDelay()\n");
    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelTtyRead(int tty_id, void *buf, int len) {
    // Get the TTY state

    // If the TTY has any line buffers, then consume as much as is there up to len
    // (could be multiple buffers, do the book keeping in Tty.c)

    // Otherwise, add proc to TTY waiting to receive queue, set tty_receive_len,
    // remove self from ready queue, and context switch!

    // Note: The following happens in a different function that is called for TRAP_TTY_RECEIVE.
    // On TRAP_TTY_RECEIVE, the corresponding TTY adds the new line buffer. If there are any
    // procs waiting to receive, it starts with the first one and goes through them. It looks
    // at their tty_receive_len, builds a new buffer holding min(tty_receive_len, chars in first line buffer) and
    // puts this min in tty_receive_len for the proc, and points the proc's tty_receive_buf to
    // this buffer. The TTY then "consumes" what it used to make this buffer, then moves
    // the buffer off of its waiting to receive queue and adds it to the ready queue.

    // When control returns here, the process copies tty_recieve_buf to buf, frees tty_receive_buf,
    // and returns tty_receive_len.
    return THEYNIX_EXIT_SUCCESS;
}

int KernelTtyWrite(int tty_id, void *buf, int len) {
    // Get the TTY state

    // Set tty_transmit_len = len

    // Copy first len chars of buf into a kernel heap string poitned to by tty_transmit_buffer
    // Set tty_transmit_pointer to tty_transmit_buffer

    // Enqueue self in waiting to transmit for TTY

    // If I'm the only one, call TtyTransmit with len = min(TERMINAL_MAX_LINE, tty_transmit_len)

    // Remove from ready queue and context switch

    // Note: The following happens in a different function that is called for TRAP_TTY_TRANSMIT.
    // Look at first proc on waiting to transmit queue. If tty_transmit_len <= TERMINAL_MAX_LINE,
    // then free tty_transmit_buffer, remove from waiting queue, and move to ready queue, then
    // transmit for next proc on waiting queue, or return if its empty. Else,
    // tty_transmit_len -= TERMINAL_MAX_LINE, tty_transmit_pointer += TERMINAL_MAX_LINE,
    // call TtyTransmit with new len and pointer, and return.

    // When control returns here, return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelPipeInit(int *pipe_idp) {
    // Make a new rod

    // Put it in the rod list

    // Put the rod id into pointer

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelPipeRead(int pipe_id, void *buf, int len) {
    // Get the pipe

    // If we have enough chars available, copy into buffer and consume those from pipe

    // Otherwise, add to the waiting to read queue for the pipe, remove from ready queue, and context switch!

    // Read from the pipe and consume the characters

    // If another proc waiting to read and enough characters available, move next to ready

    // Return num chars read (should be len?)
    return THEYNIX_EXIT_SUCCESS;
}

int KernelPipeWrite(int pipe_id, void *buf, int len) {
    // Write into pipe's buffer, expanding buffer capacity if necessary

    // If another proc is waiting and enough characters available, move him to ready

    // Return len
    return THEYNIX_EXIT_SUCCESS;
}

int KernelLockInit(int *lock_idp) {
    // Make the lock and save it in the list

    // Set lock_id

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelAcquire(int lock_id) {
    // while (!available) context switch (but stay in ready queue)

    // Set available = false
    // Set owner_id = my pid

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelRelease(int lock_id) {
    // Check owner_id == my pid, return error if false

    // Set available = true
    return THEYNIX_EXIT_SUCCESS;
}

int KernelCvarInit(int *cvar_idp) {
    // Make a CVARRRR and put it in the list

    // Set cvar_id
    return THEYNIX_EXIT_SUCCESS;
}

int KernelCvarSignal(int cvar_id) {
    // If cvar wait queue is empty, then return

    // Else, pop proc and add to ready queue
    return THEYNIX_EXIT_SUCCESS;
}

int KernelCvarBroadcast(int cvar_id) {
    // For each proc in cvar wait queue, pop and add to ready queue
    return THEYNIX_EXIT_SUCCESS;
}

int KernelCvarWait(int cvar_id, int lock_id) {
    // Make sure I own the lock
    // Release lock

    // Remove from ready queue
    // Add to CVar wait queue
    // Context switch

    // While (lock is not available)
        // Remove from ready queue
        // Add to cvar wait queue

    // Set lock.avail = false and owner is meeeee!
    return THEYNIX_EXIT_SUCCESS;
}

int KernelReclaim(int id) {
    // Find appropriate struct in kernel lists, remove from list, and freeeeeeeeeeeee
    return THEYNIX_EXIT_SUCCESS;
}
