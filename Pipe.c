#include "Pipe.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Kernel.h"
#include "Log.h"

/*
 * Pipe.h
 * Datastructure and helper methods for pipes.
 */

extern unsigned int next_synch_resource_id;

// Initialize a new pipe
Pipe *PipeNewPipe() {
    Pipe *p = calloc(1, sizeof(Pipe));
    p->id = next_synch_resource_id++;

    p->num_chars_available = 0;
    p->buffer_capacity = 0;

    p->waiting_to_read = ListNewList(0);
    return p;
}

// Reads len chars into user_buf from pipe buf
// User must ensure there are more chars available
// then needed when making this call
int PipeCopyIntoUserBuffer(Pipe *p, void *user_buf, int len) {
    assert(len <= p->num_chars_available);

    memcpy(user_buf, p->buffer_ptr, len);

    // Move pointer forward to next unconsumed
    p->buffer_ptr += len;
    p->num_chars_available -= len;
    if (0 == p->num_chars_available) {
        // No chars remaining, so reuse buffer
        p->buffer_ptr = p->buffer;
    }

    return len;
}

// Read len chars from user_buf into pipe's buffer
int PipeCopyIntoPipeBuffer(Pipe *p, void *user_buf, int len) {
    assert(len <= PipeSpotsRemaining(p));
    // buffer_ptr + num_chars = next blank addr in pipe buffer
    memcpy(p->buffer_ptr + p->num_chars_available, user_buf, len);
    p->num_chars_available += len;
    return len;
}

// Number of empty bytes between the end of the current input
// and the end of the buffer
void PipeDestroyPipe(Pipe *p) {
    // List should be empty because reclaim
    // will fail if it isn't
    assert(ListEmpty(p->waiting_to_read));

    ListDestroy(p->waiting_to_read);

    if (p->buffer) {
        free(p->buffer);
    }

    free(p);
}

// Number of empty bytes between the end of the current input
// and the end of the buffer
int PipeSpotsRemaining(Pipe *p) {
   int consumed_chars = p->buffer_ptr - p->buffer;
   return p->buffer_capacity - p->num_chars_available - consumed_chars;
}
