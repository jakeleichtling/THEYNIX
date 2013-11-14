#ifndef _LEDYARD_BRIDGE_H
#define _LEDYARD_BRIDGE_H

/* Macros */

#define MAX_CARS 3

#define TO_NORWICH 0
#define TO_HANOVER 1

/* Types */

struct DirectionNameSleep {
  int direction;
  int name;
  int sleep_duration;
};

typedef struct DirectionNameSleep DirectionNameSleep;

/* Function Prototypes */

void LedyardBridgeInit();

#endif
