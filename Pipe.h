#ifndef _PIPE_H_
#define _PIPE_H_

#include "List.h"

/*
  Code for pipes.
*/

struct Pipe {
    int id;

    int num_chars_available;
    int buffer_capacity;
    void *buffer;
    void *buffer_ptr; // where we are in the buffer

    // List of procs waiting to read from the buffer.
    // USE LEN OF BYTES TO READ FOR ID
     List *waiting_to_read;
};

typedef struct Pipe Pipe;

Pipe *PipeNewPipe();

// Reads len chars into user_buf
// User must ensure there are more chars available
// then needed when making this call
int PipeCopyIntoUserBuffer(Pipe *p, void *user_buf, int len);

// Read len chars from user_buf into pipes buffer
int PipeCopyIntoPipeBuffer(Pipe *p, void *user_buf, int len);

int PipeSpotsRemaining(Pipe *p);

void PipeDestroyPipe(Pipe *p);

#endif
