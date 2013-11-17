#include <yalnix.h>
#include <stdlib.h>

void Recurse(int current, int max) {
    if (current >= max) {
        return;
    } else {
        Recurse(++current, max);
    }
}

void BigStackVar(int size) {
    int array[size];
    (void) array[0]; // usage warning..
}

int main(int argc, char** argv) {
    TtyPrintf(1, "Hi, I'm going to test out some stack growth!\n");
    TtyPrintf(1, "Lets start with some moderate recursion...\n");
    Recurse(0, 100);

    TtyPrintf(1, "Now I'm going to get a large static array\n");
    BigStackVar(1000);

    int rc = Fork();
    if (0 == rc) {
        TtyPrintf(1, "===I'm child pid %d and I'm going to blow my stack with recursion!\n", 
            GetPid());
        Recurse(0, 999999);
    }

    int status;
    Wait(&status);
    TtyPrintf(1, "Child %d died with status %d\n", rc, status);

    rc = Fork();
    if (0 == rc) {
        TtyPrintf(1, "===I'm child pid %d and I'm going to blow my stack with an array!\n", 
            GetPid());
        char *str = calloc(10000, sizeof(char)); // use up some heap first...
        (void) str; // warning
        BigStackVar(120000);
    }
    Wait(&status);
    TtyPrintf(1, "Child %d died with status %d\n", rc, status);

    return 0;
}
