/*
  Prototypes for the Yalnix system calls.
*/

int Fork(void);

int Exec(char *filename, char **argvec);

void Exit(int status);

int Wait(int *status_ptr);

int GetPid(void);

int Brk(void *addr);

int Delay(int clock_ticks);

int TtyRead(int tty_id, void *buf, int len);

int TtyWrite(int tty_id, void *buf, int len);

int PipeInit(int *pipe_idp);

int PipeRead(int pipe_id, void *buf, int len);

int PipeWrite(int pipe_id, void *buf, int len);

int LockInit(int *lock_idp);

int Acquire(int lock_id);

int Release(int lock_id);

int CvarInit(int *cvar_idp);

int CvarSignal(int cvar_id);

int CvarBroadcast(int cvar_id);

int CvarWait(int cvar_id, int lock_id);

int Reclaim(int id);
