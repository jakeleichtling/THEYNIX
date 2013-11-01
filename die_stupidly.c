#include <hardware.h>
#include <stdlib.h>

#include "Log.h"


int main(int argc, char *argv[]) {
    int i;

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Hi, I'm die_stupidly (%d)\n", GetPid());
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "I have %d args. They are:\n", argc);
    for (i = 0; i < argc; i++) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "%s\n", argv[i]);
    }

    int rc = Fork();
    if (0 == rc) {
        Exec("orphan", NULL);
        Exit(-1); // should never happen
    }

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Now I'm going to dereference null!\n");

    char * n = (char *) NULL;
    *n = 'n';
    return 0;
}

