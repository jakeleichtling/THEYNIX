#include <hardware.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Log.h"

#define BIG_HEAP 1048576
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
    int rc;

    rc = Fork();
    // if (rc) {
    //     TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Yay! I'm the parent, and my child's PID is %d", rc);
    // } else {
    //     TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "Yay! I'm the child, and my PID is %d", GetPid());
    // }

/* Delay test:
     rc = Delay(5);
     if (THEYNIX_EXIT_SUCCESS == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for 5 ticks successful \n");
     } else {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for 5 ticks failed \n");
     }

     rc = Delay(0);
     if (THEYNIX_EXIT_SUCCESS == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for 0 ticks successful \n");
     } else {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for 0 ticks failed \n");
     }

     rc = Delay(-1);
     if (THEYNIX_EXIT_SUCCESS == rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for -1 ticks successful (should fail) \n");
     } else {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: delay for -1 ticks failed (should fail) \n");
     }

     // Malloc a ton of mem to test brk
     char *buff = calloc(BIG_HEAP, sizeof(char));
     if (buff) {
         TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: calloced %d bytes \n", BIG_HEAP);
     } else {
         TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Init: callocing %d bytes failed\n", BIG_HEAP);
     }
     free(buff);

     recurse(0);

     while (true) {
         Pause();
     }
     TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< INIT PROGRAM END \n");
*/

     return 0;
}

