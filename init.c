#include <hardware.h>
#include <stdbool.h>

#include "Log.h"

int main(int argc, char *argv[]) {
     TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Delay in init()!\n");
     Delay(5);
     while (true) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Pause loop in init()!\n");
        Pause();
    }
}
