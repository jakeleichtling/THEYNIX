#include <hardware.h>
#include <string.h>
#include <stdlib.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    int parent_to_child_pipe;
    int child_to_parent_pipe;

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I'm the parent, pid: %d\n", GetPid());

    int rc = PipeInit(NULL);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeInit with null arg: rc = %d\n", rc);
    
    rc = PipeInit(&parent_to_child_pipe);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeInit with %x: rc = %d\n", &parent_to_child_pipe, rc);
    rc = PipeInit(&child_to_parent_pipe);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeInit with %x arg: rc = %d\n", &child_to_parent_pipe, rc);

    char *parent_to_child_message = "Hello Child";
    char *child_to_parent_message = "Can I have some pizza?";

    char *parent_buff = calloc(30, sizeof(char));

    // Testing out some bad reads
    // illegal length
    rc = PipeRead(parent_to_child_pipe, parent_buff, -1);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeRead w/ len = -1: rc = %d\n", rc);

    // 0 length (should return immediately)
    rc = PipeRead(parent_to_child_pipe, parent_buff, 0);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeRead w/ len = 0: rc = %d\n", rc);

    // invalid buffer addr
    rc = PipeRead(parent_to_child_pipe, (void *) 10, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeRead w/ wacky buffer: rc = %d\n", rc);

    // invalid pipe id
    rc = PipeRead(56, parent_buff, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeRead w/ invalid pipe id: rc = %d\n", rc);

    // invalid buffer permissions (strings are read only!)
    rc = PipeRead(parent_to_child_pipe, parent_to_child_message, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeRead w/ invalid permissions buffer: rc = %d\n", rc);

    // Testing out some bad writes
    // illegal length
    rc = PipeWrite(parent_to_child_pipe, parent_to_child_message, -1);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeWrite w/ len = -1: rc = %d\n", rc);

    // 0 length (should return immediately)
    rc = PipeWrite(parent_to_child_pipe, parent_to_child_message, 0);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeWrite w/ len = 0: rc = %d\n", rc);

    // invalid buffer addr
    rc = PipeWrite(parent_to_child_pipe, (void *) 10, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeWrite w/ wacky buffer: rc = %d\n", rc);


    // invalid pipe id
    rc = PipeWrite(56, parent_to_child_message, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "PipeWrite w/ invalid pipe id: rc = %d\n", rc);


    rc = Fork();
    if (0 == rc) {
        Delay(5); // Let Parent start first
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "======I'm the child, pid %d\n", GetPid());
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "======Reading from parent_to_child_pipe (%d)\n", parent_to_child_pipe);
        char *child_buff = calloc(20, sizeof(char));
        PipeRead(parent_to_child_pipe, (void *) child_buff, 5);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "======Read 5 bytes: %s \n", child_buff);

        bzero(child_buff, 20);
        PipeRead(parent_to_child_pipe, (void *) child_buff, 6);
        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "======Reading remaining 6 bytes: %s\n", child_buff);

        TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "======Writing %s to parent\n", child_to_parent_message);
        // write with proc waiting
        PipeWrite(child_to_parent_pipe, child_to_parent_message, strlen(child_to_parent_message));

        Exit(0);
    }

    // write with no procs waiting
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "I will write \"%s\" to my child\n", parent_to_child_message);
    PipeWrite(parent_to_child_pipe, (void *) parent_to_child_message, 20);

    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reading from child_to_parent_pipe (%d) \n", child_to_parent_pipe);
    PipeRead(child_to_parent_pipe, parent_buff, strlen(child_to_parent_message));
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Read: %s \n", parent_buff);

    int status;
    Wait(&status);

    Reclaim(parent_to_child_pipe);
    Reclaim(child_to_parent_pipe);

    // Try writing to reclaimed pipe!
    rc = PipeWrite(parent_to_child_pipe, parent_to_child_message, 10);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Writing to reclaimed pipe: rc = %d\n", rc);
    

    return 0;
}
