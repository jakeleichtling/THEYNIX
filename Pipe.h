#ifndef _PIPE_H_
#define _PIPE_H_

#include "List.h"

/*
 * Pipe.h
 * Datastructure and helper methods for pipes.
 */

struct Pipe {
    int id;

    // number of bytes available to read in the pipe
    int num_chars_available;
    // size of the buffer
    int buffer_capacity;
    void *buffer;

    // points to the first unconsumed byte in the buffer
    void *buffer_ptr;

    // List of procs waiting to read from the buffer.
    // USE LEN OF BYTES TO READ FOR ID
     List *waiting_to_read;
};

typedef struct Pipe Pipe;

// Initialize a new pipe
Pipe *PipeNewPipe();

// Reads len chars into user_buf
// User must ensure there are more chars available
// then needed when making this call
int PipeCopyIntoUserBuffer(Pipe *p, void *user_buf, int len);

// Read len chars from user_buf into pipes buffer
// len must be less than the number of spots remaining in the buffer
int PipeCopyIntoPipeBuffer(Pipe *p, void *user_buf, int len);

// Number of empty bytes between the end of the current input
// and the end of the buffer
int PipeSpotsRemaining(Pipe *p);

// Free pipe structure
void PipeDestroyPipe(Pipe *p);

#endif
