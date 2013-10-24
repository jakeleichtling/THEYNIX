#ifndef _SYSTEMCALLS_H_
#define _SYSTEMCALLS_H_

#include <hardware.h>

/*
  Prototypes for the Yalnix system calls.
*/

int KernelFork(void);

int KernelExec(char *filename, char **argvec);

void KernelExit(int status);

int KernelWait(int *status_ptr);

int KernelGetPid(void);

int KernelBrk(void *addr);

int KernelDelay(int clock_ticks, UserContext *user_context);

int KernelTtyRead(int tty_id, void *buf, int len);

int KernelTtyWrite(int tty_id, void *buf, int len);

int KernelPipeInit(int *pipe_idp);

int KernelPipeRead(int pipe_id, void *buf, int len);

int KernelPipeWrite(int pipe_id, void *buf, int len);

int KernelLockInit(int *lock_idp);

int KernelAcquire(int lock_id);

int KernelRelease(int lock_id);

int KernelCvarInit(int *cvar_idp);

int KernelCvarSignal(int cvar_id);

int KernelCvarBroadcast(int cvar_id);

int KernelCvarWait(int cvar_id, int lock_id);

int KernelReclaim(int id);

#endif
