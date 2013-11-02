#include <hardware.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Log.h"

#define BIG_HEAP 1048
#define MAX_REC 100

int data;

// Recurse a bunch of times to test stack growth
void recurse(int level) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: recursion level %d \n", level);
    ++level;
    if (level < MAX_REC) {
        recurse(level);
    }
}

int main(int argc, char *argv[]) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> INIT PROGRAM START \n");

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, " Mallocing %d bytes \n", BIG_HEAP);
    char *mem = malloc(BIG_HEAP);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, " mem addr = %p \n", mem);
    free(mem);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Yay! My PID is %d\n", GetPid());
    int status;
    int wait_rc;
    wait_rc = Wait(&status);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Waited with no children. rc = %d\n", wait_rc);
    int rc = Fork();
    if (rc == 0) {
        char *args[2];
        args[0] = "bo";
        args[1] = "zo";
        Exec("die_stupidly", args);
    } else {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Wait for child to die....\n");
        Delay(2);
        Wait(&status);
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Childed exited with status: %d\n", status);
    }

    rc = Fork();
    if (rc == 0) {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "==== I'm the child %d, delaying...\n", GetPid());
        Delay(2);
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "==== I'm the child, done waiting!\n", GetPid());
        Exit(1);
    } else {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Waiting on child: %d\n", rc);
        Wait(&status);
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Child exited with status: %d\n", status);
    }
    wait_rc = Wait(&status);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Waited with no children. rc = %d\n", wait_rc);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< INIT PROGRAM END \n");

    return 0;
}

