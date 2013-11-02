

#ifndef _LOG_H_
#define _LOG_H_

#include <yalnix.h>

/*
  Logging levels for TracePrintf().
*/

#define TRACE_LEVEL_FUNCTION_INFO 4
#define TRACE_LEVEL_DETAIL_INFO 3
#define TRACE_LEVEL_NON_TERMINAL_PROBLEM 2
#define TRACE_LEVEL_TERMINAL_PROBLEM 1

/*
  Generic success and failure codes.
*/
#define THEYNIX_EXIT_SUCCESS 0
#define THEYNIX_EXIT_FAILURE -1

//#define ERROR THEYNIX_EXIT_FAILURE TODO: conflicts with yalnix.h!!!
#define SUCCESS THEYNIX_EXIT_SUCCESS


// Kill Signals
#define KILLED_OUT_OF_FRAMES -10
#define KILLED_INVALID_MEM -11
#define KILLED_TRAP_ILLEGAL -12
#define KILLED_ILL_KERNEL_MEM_ACC -13
#define KILLED_NULL -14
#define KILLED_TRAP_MATH -15
#define KILLED_TRAP_NOT_DEFINED -16
#define KILLED_KERNEL_TRAP_NOT_DEFINED -17

#endif
