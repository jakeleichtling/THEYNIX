#include "SystemCalls.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "CVar.h"
#include "LoadProgram.h"
#include "Log.h"
#include "Lock.h"
#include "Kernel.h"
#include "PMem.h"
#include "VMem.h"
#include "Pipe.h"

/*
 * SystemCalls.h
 *
 * Methods to handle all syscalls.
 * They behave (hopefully) as the spec indicates.
 */

extern List *clock_block_procs;
extern List *waiting_on_children_procs;
extern List *ready_queue;

/* Input Validate helper methods */

// For the given page, return true if it has the specified permissions
bool ValidatePage(unsigned int page, unsigned long permissions) {
    bool valid = current_proc->region_1_page_table[page].valid == 1;
    bool has_permissions = (current_proc->region_1_page_table[page].prot & permissions)
         == permissions;

    return valid && has_permissions;
}

// Starting at address arg, check that every page from arg to arg+num_bytes 
// has the specified page.
bool ValidateUserArg(unsigned int arg, int num_bytes, unsigned long permissions) {
    if (arg >= VMEM_1_LIMIT) {
        return false;
    }
    if (arg < VMEM_1_BASE) {
        return false;
    }

    // get relative page for region 1 from arg addr
    int start_page = ADDR_TO_PAGE(arg - VMEM_1_BASE);
    // how far does this memory span?
    int finish_page = start_page + (num_bytes >> PAGESHIFT);
    int i;
    for (i = start_page; i <= finish_page; i++) {
        if (!ValidatePage(i, permissions)) {
            return false;
        }
    }
    return true;
}

// validate string arg for read access
// checks every byte until nul char is found
bool ValidateUserString(char *str) {
    if (((unsigned int) str) >= VMEM_1_LIMIT) {
        return false;
    }
    if (((unsigned int) str) < VMEM_1_BASE) {
        return false;
    }

    // Validate the first page, in case no memory is valid
    int start_page = ADDR_TO_PAGE(((unsigned int) str) - VMEM_1_BASE);
    if (!ValidatePage(start_page, PROT_READ)) {
        return false;
    }
    // validate every address until null char is found
    int current_page;
    while (*str != '\0') {
       str++;
       current_page = ADDR_TO_PAGE(((unsigned int) str) - VMEM_1_BASE);
       if (!ValidatePage(current_page, PROT_READ)) {
           return false;
       }
    }
    return true;
}

int KernelFork(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelFork()\n");

    // Save the current user context.
    current_proc->user_context = *user_context;

    // Make a new child PCB with the same user context as the parent.
    PCB *child_pcb = NewBlankPCBWithPageTables(current_proc->user_context);

    if (!child_pcb) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Error creating fork child PCB.\n");
        return ERROR;
    }

    child_pcb->waiting_on_children = false;

    // copy data about address space
    child_pcb->lowest_user_stack_page = current_proc->lowest_user_stack_page;
    child_pcb->user_brk_page = current_proc->user_brk_page;

    // Copy over region 1.
    if (CopyRegion1PageTableAndData(current_proc, child_pcb) == ERROR) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Could not copy region 1 page table and data.\n");
        return ERROR;
    }

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
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelFork() [child: pid = %d] \n\n", 
            current_proc->pid);
        return 0;
    } else {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelFork() [parent: pid = %d] \n\n", 
            current_proc->pid);
        return child_pid;
    }

    return 0;
}

int KernelExec(char *filename, char **argvec, UserContext *user_context_ptr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelExec()\n");
    if (!ValidateUserString(filename)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid filename str for exec\n");
        return ERROR;
    }

    // Copy the filename string and arguments to the Kernel heap.
    int filename_len = strlen(filename);
    char *heap_filename = calloc(filename_len + 1, sizeof(char));
    strncpy(heap_filename, filename, filename_len);

    char **heap_argvec = NULL;
    int num_args = 0;

    if (argvec) {
        // Validate argvec char * array.
        char **argvec_ptr = argvec;
        while (true) {
            if (!ValidateUserArg((unsigned int) argvec_ptr, sizeof(char *), PROT_READ)) {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid string array for exec args\n");
                return ERROR;
            }

            if (!(*argvec_ptr)) { // We've reached the end of argvec.
                break;
            }

            argvec_ptr++;
        }

        int i;
        for (i = 0; argvec[i]; i++, num_args++);

        // Validate each string
        for (i = 0; i < num_args; i++) {
            if (!ValidateUserString(argvec[i])) {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid string for exec arg %d\n", i);
                return ERROR;
            }
        }

        // copy arguments into kernel heap
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

    // Create the new region 1 page table, loading the executable text from the given file.
    // LoadProgram() also frees the entire region 1 before recreating it for the new program.
    if (LoadProgram(heap_filename, heap_argvec, current_proc) == ERROR) {
      TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "LoadProgram() failed.\n");
      return ERROR;
    }

    // Now use this new user context!
    *user_context_ptr = current_proc->user_context;

    // Free the filename string and arguments in the Kernel heap.
    free(heap_filename);
    if (heap_argvec) {
        int i;
        for (i = 0; i < num_args; i++) {
            char *heap_arg = heap_argvec[i];
            free(heap_arg);
        }
        free(heap_argvec);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelExec()\n\n");

    // Return
    return SUCCESS;
}

// Documentation notes:
// If the process currently owns any locks, we will release them
void KernelExit(int status, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelExit(%p)\n", user_context);
    // If initial process, halt system
    if (current_proc->pid == INIT_PID) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Init Proc exited w/ status %d. Halting!\n", 
            status);
        Halt();
    }

    // Release system resources and free datastructures
    
    // Release any locks
    while(!ListEmpty(current_proc->owned_lock_ids)) {
        int lock_id = (int) ListPeak(current_proc->owned_lock_ids);
        KernelRelease(lock_id);
    }
    ListDestroy(current_proc->owned_lock_ids);

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

    // clean up any the rest of the buffers
    free(current_proc->tty_receive_buffer);
    free(current_proc->tty_transmit_buffer);

    // Free all frames
    FreeRegion1PageTable(current_proc);
    free(current_proc->region_1_page_table);

    FreeRegion0StackPages(current_proc);
    free(current_proc->kernel_stack_page_table);

    // If has a parent, move proc to zombie_children list of parent
    if (current_proc->live_parent) {
        ListRemoveById(current_proc->live_parent->live_children, current_proc->pid);
        ListAppend(current_proc->live_parent->zombie_children, current_proc, current_proc->pid);
        // If parent is waiting_on_children, move parent proc to ready queue
        // reset waiting_on_chilrden
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
    if (!ValidateUserArg((unsigned int) status_ptr, sizeof(int), PROT_WRITE)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid status ptr in wait\n");
        return ERROR;
    }

    // First check for any zombie children
    // If zombie children list is not empty, collect exit status of one, 
    // remove PCB from list, and free
    if (!ListEmpty(current_proc->zombie_children)) {
        PCB *child = (PCB *) ListDequeue(current_proc->zombie_children);
        *status_ptr = child->exit_status;
        // Since zombie, the page tables and children lists should have
        // already been freed, so only free PCB
        free(child);
        return SUCCESS;
    }

    // No zombies, so check for live children
    // If no live children, return error
    if (ListEmpty(current_proc->live_children)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No live or zombie children to wait for!\n");
        return ERROR;
    }

    // Block and run the next proc
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
    return SUCCESS;
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
        return ERROR;
    }

    if (new_user_brk_page > current_proc->user_brk_page) { // growing the user heap...
        int rc = MapNewRegion1Pages(current_proc, current_proc->user_brk_page,
                new_user_brk_page - current_proc->user_brk_page, PROT_READ | PROT_WRITE);
        if (rc == ERROR) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "MapNewRegion1Pages() failed.\n");
            return ERROR;
        }
    } else if (new_user_brk_page < current_proc->user_brk_page) { // shrinking the heap...
        UnmapRegion1Pages(current_proc, new_user_brk_page,
                current_proc->user_brk_page - new_user_brk_page);
    } // New page is the same as old, so do nothing!

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelBrk()\n\n");
    current_proc->user_brk_page = new_user_brk_page;
    return SUCCESS;
}

int KernelDelay(int clock_ticks, UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> KernelDelay(ticks=%d)\n", clock_ticks);
    // If clock_ticks < 0, return error
    if (clock_ticks < 0) {
        return ERROR;
    }

    // If clock_ticks == 0, return success
    if (0 == clock_ticks) {
        return SUCCESS;
    }

    // Set clock ticks remaining to clock_ticks
    current_proc->clock_ticks_until_ready = clock_ticks;

    // Put proc in list of clock blocked
    ListEnqueue(clock_block_procs, current_proc, current_proc->pid);

    SwitchToNextProc(user_context);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< KernelDelay()\n");
    // Return success
    return SUCCESS;
}

int KernelTtyRead(int tty_id, void *buf, int len, UserContext *user_context) {
    // Make sure the id exists!
    if (tty_id < 0 || tty_id >= NUM_TERMINALS) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Program tried to read from invalid term\n");
        return ERROR;
    }
    if (len < 0) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "negative print length\n");
        return ERROR;
    }
    if (!ValidateUserArg((unsigned int) buf, len, PROT_WRITE)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, 
            "Buffer passed to KernelTtyRead() is not writable by the user program.\n");
        return ERROR;
    }
    // Get the TTY state
    Tty term = ttys[tty_id];

    // If the TTY has any line buffers, then consume as much as is there up to len
    LineBuffer *lb = (LineBuffer *) ListDequeue(term.line_buffers);
    if (lb) { // at least one line waiting to be consumed
        memcpy(buf, lb->buffer, len);

        // Check if all the input was consumed or not
        if (lb->length > len) {
            // Still input remaining, so make a new line buffer to push
            // back into the terminals associated line buffers
            int leftovers_length = lb->length - len;
            char *leftovers = calloc(leftovers_length, sizeof(char));
            memcpy(leftovers, lb->buffer, leftovers_length);
            free(lb->buffer);
            lb->buffer = leftovers;
            lb->length = leftovers_length;
            ListPush(term.line_buffers, lb, 0);
            return len;
        } else {
            // Consumed all of the available input
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
    memcpy(buf, current_proc->tty_receive_buffer, current_proc->tty_receive_len);
    free(current_proc->tty_receive_buffer);

    return current_proc->tty_receive_len;
}

// "Internal" method does not validate the args! Can only be used by the Kernel!
int KernelTtyWriteInternal(int tty_id, void *buf, int len, UserContext *user_context) {
    assert(tty_id >= 0 && tty_id <= NUM_TERMINALS);
    assert(len >= 0);

    // Get the TTY state
    Tty term = ttys[tty_id];

    // Set tty_transmit_len = len
    current_proc->tty_transmit_len = len;

    // Copy first len chars of buf into a kernel heap string poitned to by tty_transmit_buffer
    // Set tty_transmit_pointer to tty_transmit_buffer
    current_proc->tty_transmit_buffer = calloc(len, sizeof(char));
    memcpy(current_proc->tty_transmit_buffer, buf, len);
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
    return len;
}

// Simply validate args then pass to internal method
int KernelTtyWrite(int tty_id, void *buf, int len, UserContext *user_context) {
    if (tty_id < 0 || tty_id >= NUM_TERMINALS) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Program tried to write to invalid term\n");
        return ERROR;
    }
    if (len < 0) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "negative print length\n");
        return ERROR;
    }
    if (!ValidateUserArg((unsigned int) buf, len, PROT_READ)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, 
            "The buffer passed to KernelTtyWrite() is not readable by the user program.\n");
        return ERROR;
    }

    return KernelTtyWriteInternal(tty_id, buf, len, user_context);
}

int KernelPipeInit(int *pipe_idp) {
    if (!ValidateUserArg((unsigned int) pipe_idp, sizeof(int), PROT_WRITE)){
        return ERROR;
    }

    // Make a new rod
    Pipe *p = PipeNewPipe();
    if (!p) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Failed to create new pipe\n");
        return ERROR;
    }

    // Put it in the rod list
    ListEnqueue(pipes, p, p->id);

    // Put the rod id into pointer
    *pipe_idp = p->id;

    // Return success
    return SUCCESS;
}

int KernelPipeRead(int pipe_id, void *buf, int len, UserContext *user_context) {
    if (len < 0) {
        return ERROR;
    }
    if (len == 0) {
        return SUCCESS;
    }

    if (!ValidateUserArg((unsigned int) buf, len, PROT_WRITE)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, 
            "The buffer passed to KernelPipeRead() is not writable by the user program.\n");
        return ERROR;
    }

    // Get the pipe
    Pipe *p = (Pipe *) ListFindById(pipes, pipe_id);
    if (!p) { // check if pipe was found
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No pipe exists for id %d\n", pipe_id);
        return ERROR;
    }

    // Block until there are enough chars available
    while (p->num_chars_available < len) {
        ListAppend(p->waiting_to_read, current_proc, len);
        SwitchToNextProc(user_context);
    }

    // Use Pipe helper method to copy from pipe into user buf
    return PipeCopyIntoUserBuffer(p, buf, len);
}

int KernelPipeWrite(int pipe_id, void *buf, int len, UserContext *user_context) {
    if (len < 0) {
        return ERROR;
    }
    if (len == 0) {
        return SUCCESS;
    }

    if (!ValidateUserArg((unsigned int) buf, sizeof(char) * len, PROT_READ)) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, 
            "The buffer passed to KernelPipeRead() is not writable by the user program.\n");
        return ERROR;
    }

    // Get the pipe
    Pipe *p = (Pipe *) ListFindById(pipes, pipe_id);
    if (!p) { // check if pipe was found
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "No pipe exists for id %d\n", pipe_id);
        return ERROR;
    }

    // Write into pipe's buffer, expanding buffer capacity if necessary

    if (len > PipeSpotsRemaining(p)) {
        // Need to increase buffer size, so make one big enough to
        // accomodate the new data and copy everything over
        int new_buffer_size = p->num_chars_available + len;
        void *new_buffer = calloc(new_buffer_size, sizeof(char));

        // copy all unconsumed data from pipe
        memcpy(new_buffer, p->buffer_ptr, p->num_chars_available);
        p->buffer_ptr = new_buffer;
        free(p->buffer);
        p->buffer = new_buffer;
        p->buffer_capacity = new_buffer_size;
    }
    assert(len <= PipeSpotsRemaining(p)); // there should now be enough space

    // copy given data into pipe
    PipeCopyIntoPipeBuffer(p, buf, len);

    // If another proc is waiting and enough characters available, move him to ready
    PCB *next_proc = (PCB *) ListFindFirstLessThanIdAndRemove(p->waiting_to_read, 
        p->num_chars_available);
    if (next_proc) {
        ListAppend(ready_queue, next_proc, next_proc->pid);
    }

    // Return len
    return len;
}

int KernelLockInit(int *lock_idp) {
    if (!ValidateUserArg((unsigned int) lock_idp, sizeof(int), PROT_WRITE)){
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, 
            "The int pointer passed to KernelLockInit() is not writable by the user process.\n");
        return ERROR;
    }

    // Make a new lock.
    Lock *lock = LockNewLock();
    if (!lock) {
        return ERROR;
    }

    // Save the lock to the list of locks.
    ListEnqueue(locks, lock, lock->id);

    // Save the lock id as a side effect.
    *lock_idp = lock->id;

    // Return success
    return SUCCESS;
}

int KernelAcquire(int lock_id, UserContext *user_context) {
    // Find the lock.
    Lock *lock = (Lock *) ListFindById(locks, lock_id);

    // If the lock didn't exist, return ERROR.
    if (!lock) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Lock %d does not exist.\n", lock_id);
        return ERROR;
    }

    // If I already have the lock, do nothing and return.
    if (lock->acquired && lock->owner_id == current_proc->pid) {
        return SUCCESS;
    }

    // If the lock is available, take it and return.
    if (!lock->acquired) {
        lock->acquired = true;
        lock->owner_id = current_proc->pid;
        ListEnqueue(current_proc->owned_lock_ids, (void *) lock->id, lock->id);
        return SUCCESS;
    }

    // Otherwise, add ourselves to waiting queue for the lock
    // and context switch.
    ListEnqueue(lock->waiting_procs, (void *) current_proc, current_proc->pid);
    SwitchToNextProc(user_context);

    // Once we return, we have the lock and are out of the waiting procs list!
    assert(lock->owner_id == current_proc->pid);
    assert(lock->acquired);
    assert(ListFindById(current_proc->owned_lock_ids, lock->id));
    return SUCCESS;
}

int KernelRelease(int lock_id) {
    // Find the lock.
    Lock *lock = (Lock *) ListFindById(locks, lock_id);

    // If the lock didn't exist, return ERROR.
    if (!lock) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Lock %d does not exist.\n", lock_id);
        return ERROR;
    }

    // Ensure that I currently own the lock.
    if (!lock->acquired || lock->owner_id != current_proc->pid) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "I don't own lock %d.\n", lock_id);
        return ERROR;
    }

    // Remove lock from list of owned
    void *released_lock = ListRemoveById(current_proc->owned_lock_ids, lock->id);
    assert(released_lock); // If it wasn't in there, something went wrong!

    // If there are no processes waiting on the lock, mark it as available and return.
    if (ListEmpty(lock->waiting_procs)) {
        lock->acquired = false;
        return SUCCESS;
    }

    // Pop a process from the waiting queue, give the lock to it, and put it on the ready queue.
    PCB *unblocked_proc = (PCB *) ListDequeue(lock->waiting_procs);
    lock->owner_id = unblocked_proc->pid;
    ListEnqueue(unblocked_proc->owned_lock_ids, (void *) lock->id, lock->id);
    ListEnqueue(ready_queue, unblocked_proc, unblocked_proc->pid);

    return SUCCESS;
}

int KernelCvarInit(int *cvar_idp) {
    if (!ValidateUserArg((unsigned int) cvar_idp, sizeof(int), PROT_READ | PROT_WRITE)) {
        return ERROR;
    }
    // Make a new cvar.
    CVar *cvar = CVarNewCVar();
    if (!cvar) {
        return ERROR;
    }

    // Save the cvar to the list of cvars.
    ListEnqueue(cvars, cvar, cvar->id);

    // Save the cvar ID as a side effect.
    *cvar_idp = cvar-> id;

    // Return success.
    return SUCCESS;
}

int KernelCvarSignal(int cvar_id) {
    // Find the cvar.
    CVar *cvar = (CVar *) ListFindById(cvars, cvar_id);

    // If the cvar didn't exist, return ERROR.
    if (!cvar) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Cvar %d does not exist.\n", cvar_id);
        return ERROR;
    }

    // If no processes are waiting on the cvar, then return.
    if (ListEmpty(cvar->waiting_procs)) {
        return SUCCESS;
    }

    // Remove a process from the waiting queue and put it on the ready queue.
    PCB *waiting_proc = ListDequeue(cvar->waiting_procs);
    ListEnqueue(ready_queue, waiting_proc, waiting_proc->pid);

    return SUCCESS;
}

int KernelCvarBroadcast(int cvar_id) {
    // Find the cvar.
    CVar *cvar = (CVar *) ListFindById(cvars, cvar_id);

    // If the cvar didn't exist, return ERROR.
    if (!cvar) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Cvar %d does not exist.\n", cvar_id);
        return ERROR;
    }

    // For each proc in cvar wait queue, remove and add to ready queue
    while (!ListEmpty(cvar->waiting_procs)) {
        PCB *waiting_proc = ListDequeue(cvar->waiting_procs);
        ListEnqueue(ready_queue, waiting_proc, waiting_proc->pid);
    }

    return SUCCESS;
}

int KernelCvarWait(int cvar_id, int lock_id, UserContext *user_context) {
    // Find the cvar.
    CVar *cvar = (CVar *) ListFindById(cvars, cvar_id);

    // If the cvar didn't exist, return ERROR.
    if (!cvar) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Cvar %d does not exist.\n", cvar_id);
        return ERROR;
    }

    // Release the lock. If I get any errors, return ERROR.
    if (KernelRelease(lock_id) == ERROR) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Releasing lock %d failed.\n", lock_id);
        return ERROR;
    }

    // Add the current proc to the cvar's list of waiting procs.
    ListEnqueue(cvar->waiting_procs, current_proc, current_proc->pid);

    // Context switch.
    SwitchToNextProc(user_context);

    // Reacquire the lock. If I get any errors, return ERROR.
    if (KernelAcquire(lock_id, user_context) == ERROR) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Acquiring lock %d failed.\n", lock_id);
        return ERROR;
    }

    return SUCCESS;
}

int KernelReclaim(int id) {
    // Find appropriate struct in kernel lists, remove from list, and freeeeeeeeeeeee
    Lock *l = ListRemoveById(locks, id);
    if (l) { // resource was lock
        if (l->acquired) { // ensure it is not currently held
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Lock acquired, can't free\n");
            return ERROR;
        }
        if (!ListEmpty(l->waiting_procs)) { // ensure no procs are waiting
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Procs waiting on lock, can't free\n");
            return ERROR;
        }
        return SUCCESS;
    }

    CVar *c = ListRemoveById(cvars, id);
    if (c) { // resource was cvar
        if (!ListEmpty(c->waiting_procs)) { // ensure no procs are waiting
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Procs waiting on cvar, can't free\n");
            return ERROR;
        }
        return SUCCESS;
    }

    Pipe *p = ListRemoveById(pipes, id);
    if (p) { //resource was pipe
        if (!ListEmpty(p->waiting_to_read)) { // ensure no one is waiting to read
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Procs waiting to read on pipe, can't free\n");
            return ERROR;
        }
        return SUCCESS;
    }

    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "%d is not a valid resource id\n", id);
    return ERROR;
}
