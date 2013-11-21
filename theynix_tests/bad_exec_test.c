/*
  Tests that the OS will properly handle bad input to Exec, such as
  an illegal filename address, an illegal argvec address, an illegal
  address in argvec, and a bad filename.
*/

#include <hardware.h>
#include <stdlib.h>
#include <yalnix.h>

#include "Log.h"

int main(int argc, char **argv) {
    // Bad filename
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Exec-ing with a bad filename...\n");
    int rc = Exec("bogus_file_name.bogus_bogus_bogus", NULL);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Illegal filename address
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Exec-ing with an illegal filename address...\n");
    rc = Exec((char *) 0, NULL);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Illegal argvec address
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "Exec-ing with an illegal argvec address...\n");
    rc = Exec("theynix_tests/bad_exec_test.c", (char **) 1);
    TracePrintf(TRACE_LEVEL_TESTING_OUTPUT, "\t--> returned %d\n", rc);

    // Illegal address in argvec

    return 0;
}
