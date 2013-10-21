

#ifndef _LOG_H_
#define _LOG_H_

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

#define ERROR THEYNIX_EXIT_FAILURE

#endif
