#include <hardware.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <yalnix.h>

#include "LedyardBridge.h"

#include "Log.h"

/* Global Variables */

int num_cars_on_bridge = 0;
int num_cars_waiting[] = { 0, 0 };
int bridge_direction;

/* Function Prototypes */
void *oneCar(void *direction_name_sleep_void_pointer);
void arriveBridge(int direction, int name);
void onBridge(int direction, int name);
void exitBridge(int direction, int name);

/* Function Implementations */

void LedyardBridgeInit() {
  CvarInit(&(cvar[0]));
  CvarInit(&(cvar[1]));

  LockInit(&mutex);
}

int main(int argc, char *argv[]) {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ New car entered main!\n");

  int direction = atoi(argv[1]);
  int name = atoi(argv[2]);
  int sleep_duration = atoi(argv[3]);

  DirectionNameSleep *direction_name_sleep = calloc(1, sizeof(DirectionNameSleep));
  direction_name_sleep->direction = direction;
  direction_name_sleep->name = name;
  direction_name_sleep->sleep_duration = sleep_duration;

  oneCar(direction_name_sleep);

  free(direction_name_sleep);

  return 0;
}

/*
 Main method for each thread.
 */
void *oneCar(void *direction_name_sleep_void_pointer) {
  DirectionNameSleep *direction_name_sleep = (DirectionNameSleep *)direction_name_sleep_void_pointer;

  int direction = direction_name_sleep->direction;
  int name = direction_name_sleep->name;
  float sleep_duration = direction_name_sleep->sleep_duration;

  arriveBridge(direction, name); // Now the car is on the bridge.

  Delay(sleep_duration);

  onBridge(direction, name);

  Delay(sleep_duration);

  exitBridge(direction, name); // Now the car is off the bridge.

  return NULL;
}

/*
 First the thread acquires the mutex. Then it checks whether it can enter the bridge
 (i.e. no cars on bridge or < MAX_CARS on bridge going in same direction). If it can,
 it gets on the bridge. Otherwise, it waits to be awoken by an exiting car.
 */
void arriveBridge(int direction, int name) {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I want to go on the bridge with direction: %d\n", name, direction);
  int rc;

  // Obtain the lock that protects the bridge state.
  rc = Acquire(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex lock failed.\n");
    exit(-1);
  }

  num_cars_waiting[direction]++;

  // Can I get on the bridge?
  while (true) {
    if (0 == num_cars_on_bridge) {
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm the first car on the bridge going in direction: %d.\n", name, direction);
      num_cars_on_bridge++;
      bridge_direction = direction;
      break;
    } else if (direction == bridge_direction && num_cars_on_bridge < MAX_CARS) {
      num_cars_on_bridge++;
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm car #%d on the bridge going in direction: %d.\n", name, num_cars_on_bridge, direction);
      break;
    } else {
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I can't get on the bridge with direction %d. the bridge has %d cars and is in direction %d.\n", name, direction, num_cars_on_bridge, bridge_direction);
      CvarWait(cvar[direction], mutex);
    }
  }

  num_cars_waiting[direction]--;

  rc = Release(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex release failed.\n");
    exit(-1);
  }
}

/*
 Do stuff while you are on the bridge!
 */
void onBridge(int direction, int name) {
  int rc;

  // Obtain the lock that protects the bridge state.
  rc = Acquire(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex lock failed.\n");
    exit(-1);
  }

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm on the bridge, yo, going in direction %d.\n", name, direction);
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ \tThere are %d cars waiting to gO to hanover and %d cars waiting to gO to norwich.\n", num_cars_waiting[TO_HANOVER], num_cars_waiting[TO_NORWICH]);

  rc = Release(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex release failed.\n");
    exit(-1);
  }
}

/*
 Get off the bridge!
 */
void exitBridge(int direction, int name) {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I want to get off the bridge with direction: %d.\n", name, direction);
  int rc;

  // Obtain the lock that protects the bridge state.
  rc = Acquire(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex lock failed.\n");
    exit(-1);
  }

  // Get off the bridge.
  bool last_car_on_bridge = (num_cars_on_bridge == 1);
  num_cars_on_bridge--;
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I got off the bridge going in direction %d. There are now %d cars on the bridge.\n",
	 name, direction, num_cars_on_bridge);

  rc = Release(mutex);
  if (rc == ERROR) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Mutex release failed.\n");
    exit(-1);
  }

  // Definitely signal the cars waiting to go in the same direction as me.
  CvarBroadcast(cvar[direction]);

  // Only signal the cars waiting to go in the other direction as me if I
  // am the last car on the bridge.
  if (last_car_on_bridge) {
    int opposite_direction = direction == TO_NORWICH ? TO_HANOVER : TO_NORWICH;
      CvarBroadcast(cvar[opposite_direction]);
  }
}
