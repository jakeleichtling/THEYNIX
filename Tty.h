#ifndef _TTY_H_
#define _TTY_H_

/*
  Code for the Yalnix TTYs.
*/

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
    PCB *waiting_to_transmit;
};

typedef struct Tty Tty;

/*
  A structure to hold a string and its length.
*/
struct LineBuffer {
    char *buffer;
    int length;
};

typedef struct LineBuffer LineBuffer;

#endif
