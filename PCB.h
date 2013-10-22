#ifndef _PCB_H
#define _PCB_H

#include <stdbool.h>

#include "hardware.h"
#include "List.h"

/* Struct */

/*
  Process Control Block: A struct for keeping track of the various data associated with a given
  user process.
*/
typedef struct PCB PCB;
struct PCB {
<<<<<<< HEAD
    unsigned int pid;

     UserContext user_context;
     KernelContext *kernel_context;
=======
    UserContext user_context;
    KernelContext *kernel_context;

    unsigned int pid;
>>>>>>> 56cabc92c8074e1a0124c8a6863217689bacd2a9

    struct pte *kernel_stack_page_table;
    struct pte *region_1_page_table;

    PCB *live_parent;
    List *live_children;
    List *zombie_children;

    bool waiting_on_children;

    int lowest_user_stack_page;
    int user_brk_page;

    int exit_status;

    int clock_ticks_until_ready;

    int tty_receive_len;
    char *tty_receive_buffer;

    int tty_transmit_len;
    char *tty_transmit_buffer;
    char *tty_transmit_pointer;

    int pipe_read_len;
};

/* Function Prototypes */

/*
  Returns a PCB with the given UserContext deep cloned and its lists initialized.
*/
PCB *NewBlankPCB(UserContext model_user_context);

void KillCurrentProc();

#endif
