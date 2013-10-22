#include "SystemCalls.h"

#include "Log.h"
#include "PMem.h"

extern UnusedFrames unused_frames;

/*
  Implementations of Yalnix system calls.
*/

int Fork(void) {
    // Make new page tables: For each valid frame, copy the frame and make the same
    // virtual address in the new page table point to the copy.

    // Copy the parent PCB

    // Get a new PID for the child

    // Add the child to the parent's child list

    // Set child's parent pointer

    // Add the child to the ready queue

    // Set PC of child proc's KernelContext to just before return

    // Compare current PID to parent PID to return correct value
    return THEYNIX_EXIT_SUCCESS;
}

int Exec(char *filename, char **argvec) {
    // Copy filename string and arguments to Kernel stack

    // Release all user frames (not just stack, everything)

    // Load text from file, mapping to new frames when necessary

    // Put arguments in the right spot

    // Set the PC to the beginning of main

    // Return
    return THEYNIX_EXIT_SUCCESS;
}

void Exit(int status) {
    // If initial process, halt system

    // Set parent pointers of children to null

    // Save exit status

    // Free all frames

    // If has a parent, move proc to zombie_children list of parent

    // If parent is waiting_on_children, move parent proc to ready queue from blocked, and reset waiting_on_chilrden

    // If doesn't have parent, free PCB

    // Context switch (do we free kernel stack frames here?)
}

int Wait(int *status_ptr) {
    // If zombie children list is not empty, collect exit status of one, remove PCB from list, and free

    // If zombie children list is empty
        // If no live children, return error
        // Else, indicate waiting_for_children, move to blocked, context switch

    // Return the exit status
    return THEYNIX_EXIT_SUCCESS;
}

int GetPid(void) {
    // Operating systems, wooooo!

    // Return the PID from current PCB
    return THEYNIX_EXIT_SUCCESS;
}

int Brk(void *addr) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> Brk(%p)\n", addr);

    unsigned int new_user_brk_page = ADDR_TO_PAGE(addr - 1) + 1;

    // Ensure we aren't imposing on user stack limits.
    if (new_user_brk_page >= current_proc->lowest_user_stack_page) {
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Address passed to Brk() (%p) does not leave a blank page between heap and user stack page (%d).\n",
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

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< Brk()\n\n");
    current_proc->user_brk_page = new_user_brk_page;
    return 0;
}

int Delay(int clock_ticks) {
    // If clock_ticks < 0, return error

    // If clock_ticks == 0, return success

    // Put proc in list of clock blocked

    // Set clock ticks remaining to clock_ticks

    // Context switch!

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int TtyRead(int tty_id, void *buf, int len) {
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

int TtyWrite(int tty_id, void *buf, int len) {
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

int PipeInit(int *pipe_idp) {
    // Make a new rod

    // Put it in the rod list

    // Put the rod id into pointer

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int PipeRead(int pipe_id, void *buf, int len) {
    // Get the pipe

    // If we have enough chars available, copy into buffer and consume those from pipe

    // Otherwise, add to the waiting to read queue for the pipe, remove from ready queue, and context switch!

    // Read from the pipe and consume the characters

    // If another proc waiting to read and enough characters available, move next to ready

    // Return num chars read (should be len?)
    return THEYNIX_EXIT_SUCCESS;
}

int PipeWrite(int pipe_id, void *buf, int len) {
    // Write into pipe's buffer, expanding buffer capacity if necessary

    // If another proc is waiting and enough characters available, move him to ready

    // Return len
    return THEYNIX_EXIT_SUCCESS;
}

int LockInit(int *lock_idp) {
    // Make the lock and save it in the list

    // Set lock_id

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int Acquire(int lock_id) {
    // while (!available) context switch (but stay in ready queue)

    // Set available = false
    // Set owner_id = my pid

    // Return success
    return THEYNIX_EXIT_SUCCESS;
}

int Release(int lock_id) {
    // Check owner_id == my pid, return error if false

    // Set available = true
    return THEYNIX_EXIT_SUCCESS;
}

int CvarInit(int *cvar_idp) {
    // Make a CVARRRR and put it in the list

    // Set cvar_id
    return THEYNIX_EXIT_SUCCESS;
}

int CvarSignal(int cvar_id) {
    // If cvar wait queue is empty, then return

    // Else, pop proc and add to ready queue
    return THEYNIX_EXIT_SUCCESS;
}

int CvarBroadcast(int cvar_id) {
    // For each proc in cvar wait queue, pop and add to ready queue
    return THEYNIX_EXIT_SUCCESS;
}

int CvarWait(int cvar_id, int lock_id) {
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

int Reclaim(int id) {
    // Find appropriate struct in kernel lists, remove from list, and freeeeeeeeeeeee
    return THEYNIX_EXIT_SUCCESS;
}
