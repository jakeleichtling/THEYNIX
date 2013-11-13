#include "SystemCalls.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "LoadProgram.h"
#include "Log.h"
#include "Kernel.h"
#include "PMem.h"
#include "VMem.h"
#include "Pipe.h"

extern UnusedFrames unused_frames;
extern List *clock_block_procs;
extern List *waiting_on_children_procs;
extern List *ready_queue;

/*
  Implementations of Yalnix system calls.
*/

// validate pointer arg
bool ValidateUserArg(unsigned int arg, int num_bytes, unsigned long permissions) {
    if (arg > VMEM_1_LIMIT) {
        return false;
    }
    if (arg < VMEM_1_BASE) {
        return false;
    }

    int start_page = ADDR_TO_PAGE(arg - VMEM_1_BASE);
    int finish_page = start_page + (num_bytes >> PAGESHIFT);
    int i;
    for (i = start_page; i <= finish_page; i++) {
        bool valid = current_proc->region_1_page_table[i].valid == 1;
        bool has_permissions = (current_proc->region_1_page_table[i].prot & permissions)
             == permissions;
        if (!(valid && has_permissions)) {
            return false;
        }
    }
    return true;
}

// validate string arg for read access
bool ValidateUserString(char *str) {
    int len = strlen(str);
    return ValidateUserArg((unsigned int) str, len * sizeof(char), PROT_READ);
}

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
    ListAppend(current_proc->live_children, child_pcb, child_pcb->pid);

    // Set child's parent pointer
    child_pcb->live_parent = current_proc;

    // Record the child's PID for later comparison.
    unsigned int child_pid = child_pcb->pid;

    // Set kernel_context_initialized to false and context switch to
    // child so that the KernelContext and kernel stack are copied from parent.
    ListAppend(ready_queue, current_proc, current_proc->pid);
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

int KernelExec(char *filename, char **argvec, UserContext *user_context_ptr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelExec()\n");
    if (!ValidateUserString(filename)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid filename str for exec\n");
        return THEYNIX_EXIT_FAILURE;
    }

    // Copy the filename string and arguments to the Kernel heap.
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 1\n");
    int filename_len = strlen(filename);
    char *heap_filename = calloc(filename_len + 1, sizeof(char));
    strncpy(heap_filename, filename, filename_len);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 2\n");
    char **heap_argvec = NULL;
    int num_args = 0;
    if (argvec) {
        int i;
        for (i = 0; argvec[i]; i++, num_args++);

        // Validate char* array
        if (!ValidateUserArg((unsigned int) argvec, num_args, PROT_READ)) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid char array for exec args\n");
            return THEYNIX_EXIT_FAILURE;
        }

        // Validate each string
        for (i = 0; i < num_args; i++) {
            if (!ValidateUserString(argvec[i])) {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid string for exec arg %d\n", i);
                return THEYNIX_EXIT_FAILURE;
            }
        }

        heap_argvec = calloc(num_args + 1, sizeof(char *));
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
    LoadProgram(heap_filename, heap_argvec, current_proc);

    // Now use this new user context!
    *user_context_ptr = current_proc->user_context;

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Mark 4\n");
    // Free the filename string and arguments in the Kernel heap.
    free(heap_filename);
    if (heap_argvec) { // why is load program consuming this???
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


void KernelExit(int status, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelExit(%p)\n", user_context);
    // If initial process, halt system
    if (current_proc->pid == INIT_PID) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Init Proc exited w/ status %d. Halting!\n", status);
        Halt();
    }

    // Empty out child lists
    while (!ListEmpty(current_proc->live_children)) {
        PCB* child = (PCB *) ListDequeue(current_proc->live_children);
        // Set parent pointers of children to null
        child->live_parent = NULL;
    }
    ListDestroy(current_proc->live_children);

    while (!ListEmpty(current_proc->zombie_children)) {
        PCB* child = (PCB *) ListDequeue(current_proc->zombie_children);
        free(child);
    }
    ListDestroy(current_proc->zombie_children);


    // Save exit status
    current_proc->exit_status = status;

    // Free all frames
    FreeRegion1PageTable(current_proc, unused_frames);
    free(current_proc->region_1_page_table);

    FreeRegion0StackPages(current_proc, unused_frames);
    free(current_proc->kernel_stack_page_table);

    // If has a parent, move proc to zombie_children list of parent
    if (current_proc->live_parent) {
        ListRemoveById(current_proc->live_parent->live_children, current_proc->pid);
        ListAppend(current_proc->live_parent->zombie_children, current_proc, current_proc->pid);
        // If parent is waiting_on_children, move parent proc to ready queue
        // from blocked, and reset waiting_on_chilrden
        if (current_proc->live_parent->waiting_on_children) {
            current_proc->live_parent->waiting_on_children = false;
            ListAppend(ready_queue, current_proc->live_parent, current_proc->live_parent->pid);
        }
    } else { // If doesn't have parent, free PCB
        free(current_proc);
    }

    // Context switch
    SwitchToNextProc(user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelExit()\n");
}

int KernelWait(int *status_ptr, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelWait(%p)\n", user_context);
    if (!ValidateUserArg((unsigned int) status_ptr, sizeof(int *), PROT_READ | PROT_WRITE)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid stats ptr in wait\n");
        return THEYNIX_EXIT_FAILURE;
    }
    // If zombie children list is not empty, collect exit status of one, remove PCB from list, and free
    if (!ListEmpty(current_proc->zombie_children)) {
        PCB *child = (PCB *) ListDequeue(current_proc->zombie_children);
        *status_ptr = child->exit_status;
        // Since zombie, the page tables and children lists should have
        // already been freed, so only free PCB
        free(child);
        return THEYNIX_EXIT_SUCCESS;
    }

    // If no live children, return error
    if (ListEmpty(current_proc->live_children)) {
        return THEYNIX_EXIT_FAILURE;
    }
    current_proc->waiting_on_children = true;
    SwitchToNextProc(user_context);

    // When executed again, some child must have died and put us on ready queue
    PCB *child = (PCB *) ListDequeue(current_proc->zombie_children);
    assert(child);
    *status_ptr = child->exit_status;
    // Since zombie, the page tables and children lists should have
    // already been freed, so only free PCB
    free(child);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelWait()\n");
    // Return the exit status
    return THEYNIX_EXIT_SUCCESS;
}

int KernelGetPid(void) {
    return current_proc->pid;
}

int KernelBrk(void *addr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelBrk(%p)\n", addr);

    unsigned int new_user_brk_page = ADDR_TO_PAGE(addr - 1) + 1 - REGION_1_BASE_PAGE;

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

int KernelTtyRead(int tty_id, void *buf, int len, UserContext *user_context) {
    if (tty_id < 0 || tty_id >= NUM_TERMINALS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Program tried to read from invalid term\n");
        return THEYNIX_EXIT_FAILURE;
    }
    if (len < 0) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "negative print length\n");
        return THEYNIX_EXIT_FAILURE;
    }
    if (!ValidateUserArg((unsigned int) buf, len, PROT_READ | PROT_WRITE)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid read string\n");
        return THEYNIX_EXIT_FAILURE;
    }
    // Get the TTY state
    Tty term = ttys[tty_id];

    // If the TTY has any line buffers, then consume as much as is there up to len
    int consumed = 0;
    LineBuffer *lb = (LineBuffer *) ListDequeue(term.line_buffers);
    if (lb) { // at least one line waiting to be consumed
        strncpy(buf, lb->buffer, len);
        if (lb->length > len) {
            int leftovers_length = lb->length - len;
            char *leftovers = calloc(leftovers_length, sizeof(char));
            strncpy(leftovers, lb->buffer + consumed, leftovers_length);
            free(lb->buffer);
            lb->buffer = leftovers;
            lb->length = leftovers_length;
            ListPush(term.line_buffers, lb, 0);

            return len;
        } else {
            int copied = lb->length;
            free(lb->buffer);
            free(lb);
            return copied;
        }
    }
    // Otherwise, add proc to TTY waiting to receive queue, set tty_receive_len,
    // alloc receive buffer, and context switch!
    ListEnqueue(term.waiting_to_receive, current_proc, current_proc->pid);
    current_proc->tty_receive_len = len;
    current_proc->tty_receive_buffer = calloc(len, sizeof(char));
    SwitchToNextProc(user_context);

    // When control returns here, the process copies tty_recieve_buf to buf, frees tty_receive_buf,
    // and returns tty_receive_len.
    strncpy(buf, current_proc->tty_receive_buffer, len);
    int copied;
    if (current_proc->tty_receive_len > len) {
        copied = len;
    } else {
        copied = current_proc->tty_receive_len;
    }
    free(current_proc->tty_receive_buffer);

    return copied;
}

int KernelTtyWrite(int tty_id, void *buf, int len, UserContext *user_context) {
    if (tty_id < 0 || tty_id >= NUM_TERMINALS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Program tried to write to invalid term\n");
        return THEYNIX_EXIT_FAILURE;
    }
    if (len < 0) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "negative print length\n");
        return THEYNIX_EXIT_FAILURE;
    }
    if (!ValidateUserArg((unsigned int) buf, len, PROT_READ)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid write string\n");
        return THEYNIX_EXIT_FAILURE;
    }

    // Get the TTY state
    Tty term = ttys[tty_id];

    // Set tty_transmit_len = len
    current_proc->tty_transmit_len = len;

    // Copy first len chars of buf into a kernel heap string poitned to by tty_transmit_buffer
    // Set tty_transmit_pointer to tty_transmit_buffer
    current_proc->tty_transmit_buffer = calloc(len, sizeof(char));
    strncpy(current_proc->tty_transmit_buffer, buf, len);
    current_proc->tty_transmit_pointer = current_proc->tty_transmit_buffer;

    bool queue_prev_empty = ListEmpty(term.waiting_to_transmit);

    // Enqueue self in waiting to transmit for TTY
    ListEnqueue(term.waiting_to_transmit, current_proc, current_proc->pid);

    // If I'm the only one, call TtyTransmit with len = min(TERMINAL_MAX_LINE, tty_transmit_len)
    if (queue_prev_empty) {
        if (TERMINAL_MAX_LINE < len) {
            TtyTransmit(tty_id, current_proc->tty_transmit_buffer, TERMINAL_MAX_LINE);
        } else {
            TtyTransmit(tty_id, current_proc->tty_transmit_buffer, len);
        }
    }

    // Remove from ready queue and context switch
    SwitchToNextProc(user_context);

    // When control returns here, return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelPipeInit(int *pipe_idp) {
    if (!ValidateUserArg((unsigned int) pipe_idp, sizeof(int), PROT_READ | PROT_WRITE)){
        return THEYNIX_EXIT_FAILURE;
    }
    
    // Make a new rod
    Pipe *p = PipeNewPipe();

    // Put it in the rod list
    ListEnqueue(pipes, p, p->id);

    // Put the rod id into pointer
    *pipe_idp = p->id;

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int KernelPipeRead(int pipe_id, void *buf, int len, UserContext *user_context) {
    if (len <= 0) {
        return THEYNIX_EXIT_FAILURE;
    }

    if (!ValidateUserArg((unsigned int) buf, sizeof(char) * len, PROT_WRITE)) {
        return THEYNIX_EXIT_FAILURE;
    }

    // Get the pipe
    Pipe *p = (Pipe *) ListFindById(pipes, pipe_id);
    if (!p) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No pipe exists for id %d\n", pipe_id);
        return THEYNIX_EXIT_FAILURE;
    }

    // Block until there are enough chars available
    while (p->num_chars_available < len) {
        ListAppend(p->waiting_to_read, current_proc, len);
        SwitchToNextProc(user_context);
    }
    return PipeCopyIntoUserBuffer(p, (char *) buf, len);
}

int KernelPipeWrite(int pipe_id, void *buf, int len, UserContext *user_context) {
    if (len <= 0) {
        return THEYNIX_EXIT_FAILURE;
    }

    if (!ValidateUserArg((unsigned int) buf, sizeof(char) * len, PROT_READ)) {
        return THEYNIX_EXIT_FAILURE;
    }

    // Get the pipe
    Pipe *p = (Pipe *) ListFindById(pipes, pipe_id);
    if (!p) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No pipe exists for id %d\n", pipe_id);
        return THEYNIX_EXIT_FAILURE;
    }

    // Write into pipe's buffer, expanding buffer capacity if necessary
    
    if (len > PipeSpotsRemaining(p)) { // need to increase size of buffer
        int new_buffer_size = p->num_chars_available + len;
        char *new_buffer = calloc(new_buffer_size, sizeof(char));
        strncpy(new_buffer, p->buffer_ptr, p->num_chars_available);
        p->buffer_ptr = new_buffer + p->num_chars_available;
        free(p->buffer);
        p->buffer = new_buffer;
        p->buffer_capacity = new_buffer_size;
    }
    assert(len <= PipeSpotsRemaining(p));

    PipeCopyIntoPipeBuffer(p, (char *) buf, len);

    // If another proc is waiting and enough characters available, move him to ready
    PCB *next_proc = (PCB *) ListFindFirstLessThanIdAndRemove(p->waiting_to_read, p->num_chars_available);
    if (next_proc) {
        ListAppend(ready_queue, next_proc, next_proc->pid);
    }

    // Return len
    return len;
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
