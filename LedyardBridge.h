/* Macros */

#define MAX_CARS 3

#define TO_NORWICH 0
#define TO_HANOVER 1

/* Types */

typedef struct {
  int direction;
  int name;
  int sleep_duration;
} DirectionNameSleep;

/* Function Prototypes */

void ledyardBridgeInit();
void *oneCar(void *direction_name_sleep);
