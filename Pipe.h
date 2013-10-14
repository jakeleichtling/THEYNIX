#ifndef _PIPE_H_
#define _PIPE_H_

/*
  Code for pipes.
*/

struct Pipe {
    int id;

    int num_chars_available;
    int buffer_capacity;
    void *buffer;

    // List of procs waiting to read from the buffer.
     List *waiting_to_read;
};

typedef struct Pipe Pipe;

#endif
