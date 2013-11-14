#include "Pipe.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Kernel.h"

extern unsigned int next_synch_resource_id;

Pipe *PipeNewPipe() {
    Pipe *p = calloc(1, sizeof(Pipe));
    p->id = next_synch_resource_id++;
    p->waiting_to_read = ListNewList();
    return p;
}

int PipeCopyIntoUserBuffer(Pipe *p, char *user_buf, int len) {
    assert(len <= p->num_chars_available);
    strncpy(user_buf, p->buffer_ptr, len);
    p->buffer_ptr += len;
    p->num_chars_available -= len;
    if (0 == p->num_chars_available) { // no chars remaining
        p->buffer_ptr = p->buffer;
    }
    return len;
}

int PipeCopyIntoPipeBuffer(Pipe *p, char *user_buf, int len) {
    assert(len <= PipeSpotsRemaining(p));
    strncpy(p->buffer_ptr + p->num_chars_available, user_buf, len);
    p->num_chars_available += len;
    return len;
}

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

int PipeSpotsRemaining(Pipe *p) {
   int consumed_chars = p->buffer_ptr - p->buffer;
   return p->buffer_capacity - p->num_chars_available - consumed_chars;
}
