#ifndef _TTY_H_
#define _TTY_H_

#include "List.h"

/*
 * Tty.h
 * Datastructure and helper methods for managing terminals.
 */

typedef struct Tty Tty;
typedef struct LineBuffer LineBuffer;


#define WAITING_TO_TRANSMIT_HASH_SIZE 20

/*
  A structure to keep track of the TTY state, as well as processes that are blocked on it.
*/
struct Tty {
    // A list of LineBuffers received via TRAP_TTYP_RECEIVE interrupts.
    List *line_buffers;

    // A list of procs waiting to consume input.
    List *waiting_to_receive;

    // A list of procs waiting to transmit.
    // The first proc has transmitted and is waiting for a TRAP_TTY_TRANSMIT interrupt.
    List *waiting_to_transmit;
};

/*
  A structure to hold a string and its length.
*/
struct LineBuffer {
    char *buffer;
    int length;
};

/* Function Prototypes */

/*
  Initializes allocated memory pointed to by the given Tty *.
*/
void TtyInit(Tty *tty);

#endif
