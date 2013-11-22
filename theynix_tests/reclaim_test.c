#include <stdlib.h>
#include <hardware.h>

#include "Log.h"

int main(int argc, char **argv) {
    int rc;
    int lock_id;
    int pipe_id;
    int cvar_id;

    LockInit(&lock_id);
    CvarInit(&cvar_id);
    PipeInit(&pipe_id);

    // reclaim lock
    rc = Reclaim(lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reclaim valid lock: rc = %d\n", rc);

    // reclaim cvar
    rc = Reclaim(cvar_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reclaim valid cvar: rc = %d\n", rc);

    // reclaim pipe
    rc = Reclaim(pipe_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reclaim valid pipe: rc = %d\n", rc);

    // try releasing lock again
    rc = Reclaim(lock_id);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Repeating lock reclaim: rc = %d\n", rc);

    // try negative id
    rc = Reclaim(-3478);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reclaim w/ negative id: rc = %d\n", rc);

    // bogus positive id
    rc = Reclaim(3478);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Reclaim w/ bogus positive id: rc = %d\n", rc);

    return 0;
}
