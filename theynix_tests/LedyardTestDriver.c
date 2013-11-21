#include <hardware.h>
#include <stdio.h>
#include <stdlib.h>
#include <yalnix.h>

#include "LedyardBridge.h"

#include "Log.h"

// Awakens cars waiting to get on the bridge.
extern int cvar[2];

// Protects the bridge state.
extern int mutex;

int pipe_id;

/* Function Prototypes */

void testCase0();
void testCase1();

/* Function Implementations */

int main(int argc, char **argv) {
  CvarInit(&(cvar[0]));
  CvarInit(&(cvar[1]));

  PipeInit(&pipe_id);
  Bridge *b = calloc(1, sizeof(Bridge));
  PipeWrite(pipe_id, (void *)b, sizeof(Bridge));

  LockInit(&mutex);

  testCase0();
  // testCase1();

  return 0;
}

/*
 5 cars going in each direction, all released at the same time.
 */
void testCase0() {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "5 cars going in each direction, all released at the same time,\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "\tsleeping for 1 second after getting on, and another second before getting off:\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");

  int num_cars_to_hanover = 5;
  int num_cars_to_norwich = 5;

  char *pipe_id_str = calloc(16, sizeof(char));
  sprintf(pipe_id_str, "%d\0", pipe_id);

  char *lock_id_str = calloc(16, sizeof(char));
  sprintf(lock_id_str, "%d\0", mutex);

  char *cvar_0_id_str = calloc(16, sizeof(char));
  sprintf(cvar_0_id_str, "%d\0", cvar[0]);

  char *cvar_1_id_str = calloc(16, sizeof(char));
  sprintf(cvar_1_id_str, "%d\0", cvar[1]);

  DirectionNameSleep *direction_names[num_cars_to_hanover + num_cars_to_norwich];

  int rc;

  int i;
  for (i = 0; i < num_cars_to_hanover; i++) {
    direction_names[i] = (DirectionNameSleep *)malloc(sizeof(DirectionNameSleep));
    direction_names[i]->direction = TO_HANOVER;
    direction_names[i]->name = i;
    direction_names[i]->sleep_duration = 1;

    char *direction = calloc(16, sizeof(char));
    char *name = calloc(16, sizeof(char));
    char *sleep_duration = calloc(16, sizeof(char));

    sprintf(direction, "%d\0", direction_names[i]->direction);
    sprintf(name, "%d\0", direction_names[i]->name);
    sprintf(sleep_duration, "%d\0", direction_names[i]->sleep_duration);
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Sleep duration set for %s --> %s\n", name, sleep_duration);

    char *argvec[] = { "LedyardBridge", direction, name, sleep_duration, pipe_id_str,
        lock_id_str, cvar_0_id_str, cvar_1_id_str,  NULL };
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Forking car %d.\n", i);
    rc = Fork();
    if (rc == 0) {
      rc = Exec("theynix_tests/LedyardBridge", argvec);
      if (rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Car %d exec failed.\n", i);
        exit(-1);
      }
    }

    free(direction);
    free(name);
    free(sleep_duration);
  }

  for (; i < num_cars_to_hanover + num_cars_to_norwich; i++) {
    direction_names[i] = malloc(sizeof(DirectionNameSleep));
    direction_names[i]->direction = TO_NORWICH;
    direction_names[i]->name = i;
    direction_names[i]->sleep_duration = 1;

    char direction[16];
    char *name = calloc(16, sizeof(char));
    char *sleep_duration = calloc(16, sizeof(char));

    sprintf(direction, "%d\0", direction_names[i]->direction);
    sprintf(name, "%d\0", direction_names[i]->name);
    sprintf(sleep_duration, "%d\0", direction_names[i]->sleep_duration);
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Sleep duration set for %s --> %s\n", name, sleep_duration);

    char *argvec[] = { "LedyardBridge", direction, name, sleep_duration, pipe_id_str,
        lock_id_str, cvar_0_id_str, cvar_1_id_str,  NULL };
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Forking car %d.\n", i);
    rc = Fork();
    if (rc == 0) {
    rc = Exec("LedyardBridge", argvec);
      if (rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Car %d exec failed.\n", i);
        exit(-1);
      }
    }
    free(direction);
    free(name);
    free(sleep_duration);
  }

  // Wait for all the threads and free their DirectionNames
  for (i = 0; i < num_cars_to_hanover + num_cars_to_norwich; i++) {
    Wait(&rc);

    if (rc) {
       TracePrintf(TRACE_LEVEL_DETAIL_INFO, "pthread_join failed at i = %d\n", i);
      exit(-1);
    }

    free(direction_names[i]);
  }

  free(lock_id_str);
  free(pipe_id_str);
  free(cvar_0_id_str);
  free(cvar_1_id_str);

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");
}

void testCase1() {
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "10 cars going in random directions, all released at the same time,\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "\tsleeping for 0 <= t <= 5 seconds after getting on, and another t seconds before getting off:\n");
  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");

  int num_cars = 10;
  DirectionNameSleep *direction_names[num_cars];

  int min_sleep_duration = 0;
  int max_sleep_duration = 5;

  int rc;

  int i;
  for (i = 0; i < num_cars; i++) {
    direction_names[i] = (DirectionNameSleep *)malloc(sizeof(DirectionNameSleep));
    direction_names[i]->direction = rand() % 2;
    direction_names[i]->name = i;
    direction_names[i]->sleep_duration = rand() % (max_sleep_duration + 1 - min_sleep_duration) + min_sleep_duration;

    char direction[16];
    char name[16];
    char sleep_duration[16];

    sprintf(direction, "%d\0", direction_names[i]->direction);
    sprintf(name, "%d\0", direction_names[i]->name);
    sprintf(sleep_duration, "%d\0", direction_names[i]->sleep_duration);
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "~~~ Sleep duration set for %s --> %s\n", name, sleep_duration);

    char *argvec[] = { "LedyardBridge", direction, name, sleep_duration };
    rc = Fork();
    if (rc == 0) {
      rc = Exec("theynix_tests/LedyardBridge", argvec);
      if (rc) {
        TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Car %d exec failed.\n", i);
        exit(-1);
      }
    }
  }

  // Wait for all the threads and free their DirectionNames
  for (i = 0; i < num_cars; i++) {
    Wait(&rc);

    if (rc) {
       TracePrintf(TRACE_LEVEL_DETAIL_INFO, "pthread_join failed at i = %d\n", i);
      exit(-1);
    }

    free(direction_names[i]);
  }

  TracePrintf(TRACE_LEVEL_DETAIL_INFO, "---------------------------------------------------------------------------------------\n");
}
