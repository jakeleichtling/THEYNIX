#include <hardware.h>
#include <stdlib.h>

#include "Log.h"

int main(int argc, char *argv[]) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> ORPHAN PROGRAM START \n");
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Waiting for my parent to die :'(\n");
    Delay(3);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< ORPHAN PROGRAM END \n");
    return 0;
}
