#include <hardware.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <yalnix.h>
#include <string.h>

#include "LedyardBridge.h"

#include "Log.h"

/* Global Variables */

Bridge *b;

/* Function Prototypes */
void *oneCar(void *direction_name_sleep_void_pointer);
void arriveBridge(int direction, int name);
void onBridge(int direction, int name);
void exitBridge(int direction, int name);

void bridgeLockAcquire(int mutex);
void bridgeLockRelease(int mutex);
void bridgeCvarWait(int mutex, int lock);
void writeBridgeToPipe();
void readBridgeFromPipe();

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
  pipe_id = atoi(argv[4]);
  mutex = atoi(argv[5]);
  cvar[0] = atoi(argv[6]);
  cvar[1] = atoi(argv[7]);

  b = calloc(1, sizeof(Bridge));

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

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Delaying for %d.\n", name, sleep_duration);
  Delay(sleep_duration);

  onBridge(direction, name);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Delaying for %d.\n", name, sleep_duration);
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
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: >>> arriveBridge()\n", name);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I want to go on the bridge with direction: %d\n", name, direction);

  // Obtain the lock that protects the bridge state.
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Acquiring lock.\n", name);
  bridgeLockAcquire(mutex);

  b->num_cars_waiting[direction]++;

  // Can I get on the bridge?
  while (true) {
    if (0 == b->num_cars_on_bridge) {
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm the first car on the bridge going in direction: %d.\n", name, direction);
      b->num_cars_on_bridge++;
      b->bridge_direction = direction;
      break;
    } else if (direction == b->bridge_direction && b->num_cars_on_bridge < MAX_CARS) {
      b->num_cars_on_bridge++;
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm car #%d on the bridge going in direction: %d.\n", name, b->num_cars_on_bridge, direction);
      break;
    } else {
      TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I can't get on the bridge with direction %d. the bridge has %d cars and is in direction %d.\n", name, direction, b->num_cars_on_bridge, b->bridge_direction);
      bridgeCvarWait(cvar[direction], mutex);
    }
  }

  b->num_cars_waiting[direction]--;

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Releasing the lock.\n", name);
  bridgeLockRelease(mutex);
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Lock released.\n", name);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: <<< arriveBridge()\n\n", name);
}

/*
 Do stuff while you are on the bridge!
 */
void onBridge(int direction, int name) {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: >>> onBridge()\n", name);

  // Obtain the lock that protects the bridge state.
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Acquiring lock.\n", name);
  bridgeLockAcquire(mutex);
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Lock acquired.\n", name);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I'm on the bridge, yo, going in direction %d.\n", name, direction);
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ \tThere are %d cars waiting to go to hanover and %d cars waiting to go to norwich.\n", b->num_cars_waiting[TO_HANOVER], b->num_cars_waiting[TO_NORWICH]);

  bridgeLockRelease(mutex);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: <<< onBridge()\n\n", name);
}

/*
 Get off the bridge!
 */
void exitBridge(int direction, int name) {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: >>> exitBridge()\n", name);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I want to get off the bridge with direction: %d.\n", name, direction);

  // Obtain the lock that protects the bridge state.
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Acquiring lock.\n", name);
  bridgeLockAcquire(mutex);
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: Lock acquired.\n", name);

  // Get off the bridge.
  bool last_car_on_bridge = (b->num_cars_on_bridge == 1);
  b->num_cars_on_bridge--;
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: I got off the bridge going in direction %d. There are now %d cars on the bridge.\n",
	 name, direction, b->num_cars_on_bridge);

  bridgeLockRelease(mutex);

  // Definitely signal the cars waiting to go in the same direction as me.
  CvarBroadcast(cvar[direction]);

  // Only signal the cars waiting to go in the other direction as me if I
  // am the last car on the bridge.
  if (last_car_on_bridge) {
    int opposite_direction = direction == TO_NORWICH ? TO_HANOVER : TO_NORWICH;
      CvarBroadcast(cvar[opposite_direction]);
  }

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ %d: <<< exitBridge()\n\n", name);
}

void bridgeLockAcquire(int mutex) {
    int rc = Acquire(mutex);
    if (rc != THEYNIX_EXIT_SUCCESS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "FAILED TO ACQUIRE LOCK\n");
        Exit(-1);
    }
    readBridgeFromPipe();
}

void bridgeLockRelease(int mutex) {
    writeBridgeToPipe();
    int rc = Release(mutex);
    if (rc != THEYNIX_EXIT_SUCCESS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "FAILED TO RELEASE LOCK\n");
        Exit(-1);
    }
}

void bridgeCvarWait(int mutex, int lock) {
    writeBridgeToPipe();
    int rc = CvarWait(mutex, lock);
    if (rc != THEYNIX_EXIT_SUCCESS) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "CVAR WAIT FAILED\n");
        Exit(-1);
    }
    readBridgeFromPipe();
}

void writeBridgeToPipe() {
    int rc = PipeWrite(pipe_id, (void *) b, sizeof(Bridge));
    if (rc != sizeof(Bridge)) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "FAILED TO WRITE BRIDGE\n");
        Exit(-1);
    }
}

void readBridgeFromPipe() {
    memset((void*) b, '\0', sizeof(Bridge));
    int rc = PipeRead(pipe_id, (void *) b, sizeof(Bridge));
    if (rc != sizeof(Bridge)) {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "FAILED TO READ BRIDGE\n");
        Exit(-1);
    }
}
