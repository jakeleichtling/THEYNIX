#include <hardware.h>
#include <string.h>
#include <stdlib.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    int parent_to_child_pipe;
    int child_to_parent_pipe;

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I'm the parent, pid: %d\n", GetPid());

    PipeInit(&parent_to_child_pipe);
    PipeInit(&child_to_parent_pipe);

    char *parent_to_child_message = "Hello Child";
    char *child_to_parent_message = "Can I have some pizza?";

    int rc = Fork();
    if (0 == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "======I'm the child, pid %d\n", GetPid());
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "======Reading from parent_to_child_pipe (%d)\n", parent_to_child_pipe);
        char *child_buff = calloc(20, sizeof(char));
        PipeRead(parent_to_child_pipe, (void *) child_buff, 5);
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "======Read 5 bytes: %s \n", child_buff);

        bzero(child_buff, 20);
        PipeRead(parent_to_child_pipe, (void *) child_buff, 6);
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "======Reading remaining 6 bytes: %s\n", child_buff);

        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "======Writing %s to parent\n", child_to_parent_message);
        PipeWrite(child_to_parent_pipe, child_to_parent_message, strlen(child_to_parent_message));

        Exit(0);
    }
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I will write \"%s\" to my child\n", parent_to_child_message);
    PipeWrite(parent_to_child_pipe, (void *) parent_to_child_message, 20);

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Reading from child_to_parent_pipe (%d) \n", child_to_parent_pipe);
    char *parent_buff = calloc(30, sizeof(char));
    PipeRead(child_to_parent_pipe, parent_buff, strlen(child_to_parent_message));
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Read: %s \n", parent_buff);

    int status;
    Wait(&status);

    return 0;
}
