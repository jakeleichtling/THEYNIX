#ifndef _PCB_H
#define _PCB_H

#include <stdbool.h>

#include "hardware.h"
#include "List.h"
#include "PMem.h"

/*
 * PCB.h
 * Data structure for process control blocks
 */

#define NUM_KERNEL_PAGES KERNEL_STACK_MAXSIZE / PAGESIZE

#define OWNED_LOCK_HASH_SIZE 10
#define CHILD_LIST_HASH_SIZE 10

/* Struct */

/*
  Process Control Block: A struct for keeping track of the various data associated with a given
  user process.
*/
typedef struct PCB PCB;
struct PCB {
    unsigned int pid;

    // when loading a program
    // for the first time, need to set up kernel
    // context as copy of currently running
    bool kernel_context_initialized;

    UserContext user_context;
    KernelContext kernel_context;

    struct pte *kernel_stack_page_table;
    struct pte *region_1_page_table;

    // Null if parent died
    PCB *live_parent;

    List *live_children;

    // children waiting to be collected
    List *zombie_children;

    // Locks this proc has acquired
    List *owned_lock_ids;

    // Called wait, but no children had died
    bool waiting_on_children;

    int lowest_user_stack_page;
    int user_brk_page;

    int exit_status;

    // After Delay, this field tracks
    // how many clock ticks are left for the process
    int clock_ticks_until_ready;

    // The number of bytes this proc is waiting to recieve
    // from the terminal
    int tty_receive_len;
    // Where the terminal input will be copied until the user
    // can retrieve it
    char *tty_receive_buffer;

    // The number of bytes this proc wants to transmit
    int tty_transmit_len;
    // Buffer in kernel space that holds the stuff we are waiting to transmit
    char *tty_transmit_buffer;
    // Pointer into the buffer to track how much has been written out thus far.
    // The first unwritten byte is at this pointer
    char *tty_transmit_pointer;

    // Number of bytes we are waiting to read from the pipe
    int pipe_read_len;
};

/* Function Prototypes */

/*
  Returns a PCB with the given UserContext deep cloned and its lists initialized.
*/
PCB *NewBlankPCB(UserContext model_user_context);

/*
  Same as above but allocates page tables and frames for kernel stack. Returns NULL if there are
  not enough physical frames to complete this request.
*/
PCB *NewBlankPCBWithPageTables(UserContext model_user_context);

#endif
