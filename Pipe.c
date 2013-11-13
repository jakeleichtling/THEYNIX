#include "Pipe.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

unsigned int next_pipe_id = 0;

Pipe *PipeNewPipe() {
    Pipe *p = calloc(1, sizeof(Pipe));
    p->id = next_pipe_id++;
    p->waiting_to_read = ListNewList();
    return p;
}

int PipeCopyIntoBuffer(Pipe *p, char *user_buf, int len) {
    assert(len <= p->num_chars_available);
    strncpy(user_buf, p->buffer_ptr, len);
    p->buffer_ptr += len;
    p->num_chars_available -= len;
    if (0 == p->num_chars_available) { // no chars remaining
        p->buffer_ptr = p->buffer;
    }
    return len;
}

void PipeDestroyPipe(Pipe *p) {
    while (!ListEmpty(p->waiting_to_read)) {
        ListDequeue(p->waiting_to_read); // Empty list
    }

    ListDestroy(p->waiting_to_read);

    if (p->buffer) {
        free(p->buffer);
    }

    free(p);
}

int PipeSpotsRemaining(Pipe *p) {
   return p->buffer_capacity - p->num_chars_available - (p->buffer_ptr - p->buffer);
}
