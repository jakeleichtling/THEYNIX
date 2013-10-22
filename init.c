#include <stdbool.h>

#include "Log.h"

int main(int argc, char *argv[]) {
     while (true) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Pause loop in main()!")
        Pause();
    }
}
