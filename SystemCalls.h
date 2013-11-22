#ifndef _SYSTEMCALLS_H_
#define _SYSTEMCALLS_H_

#include <hardware.h>

/*
 * SystemCalls.h
 *
 * Methods to handle all syscalls.
 * They behave (hopefully) as to spec unless noted otherwise.
 */

int KernelFork(UserContext *user_context);

int KernelExec(char *filename, char **argvec, UserContext *user_context_ptr);

// We have added the specification that a process releases any system resources
// on exit (e.g. held locks)
void KernelExit(int status, UserContext *user_context);

int KernelWait(int *status_ptr, UserContext *user_context);

int KernelGetPid(void);

int KernelBrk(void *addr);

int KernelDelay(int clock_ticks, UserContext *user_context);

int KernelTtyRead(int tty_id, void *buf, int len, UserContext *user_context);

// "Internal" method does not validate the args! Can only be used by the Kernel!
int KernelTtyWriteInternal(int tty_id, void *buf, int len, UserContext *user_context);

int KernelTtyWrite(int tty_id, void *buf, int len, UserContext *user_context);

int KernelPipeInit(int *pipe_idp);

int KernelPipeRead(int pipe_id, void *buf, int len, UserContext *user_context);

int KernelPipeWrite(int pipe_id, void *buf, int len, UserContext *user_context);

int KernelLockInit(int *lock_idp);

int KernelAcquire(int lock_id, UserContext *user_context);

int KernelRelease(int lock_id);

int KernelCvarInit(int *cvar_idp);

int KernelCvarSignal(int cvar_id);

int KernelCvarBroadcast(int cvar_id);

int KernelCvarWait(int cvar_id, int lock_id, UserContext *user_context);

int KernelReclaim(int id);

#endif
