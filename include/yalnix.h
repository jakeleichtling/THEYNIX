/* 
 * Yalnix Support Software for Linux/x86 
 *
 * Original SunOS/SPARC version by David Johnson, CMU/Rice. dbj@cs.rice.edu
 * 
 * Subsequently ported to Solaris/SPARC by the infamous Juan Leon
 *
 * Ported to Linux/x86 by S.W. Smith, Dartmouth College.  Summer 2001
 * sws@cs.dartmouth.edu
 * (with help from David Johnson and Evan Knop)
 * 
 * "LINUX" compile flag == Linux AND x86
 * Linux version must support makecontext/getcontext... e.g., >= 2.4.8
 *
 * This file: external definitions for the Yalnix kernel user interface.
 *
 * Port: new way of encoding system calls, and a "Nop" call for testing
 * the hardware support.
 *
 *
 * modified oct 2002: custom syscalls  sws
 *
 * modified oct 2008: pipes  sws
 */

#ifndef	_yalnix_h
#define	_yalnix_h




/*
 * Define the kernel call number for each of the supported kernel calls.
 */
#ifndef LINUX
//-----------Solaris version-------------------------------
#define	YALNIX_FORK		1
#define	YALNIX_EXEC		2
#define	YALNIX_EXIT		3
#define	YALNIX_WAIT		4
#define YALNIX_GETPID           5
#define	YALNIX_BRK		6
#define	YALNIX_DELAY		7

#define	YALNIX_TTY_READ		21
#define	YALNIX_TTY_WRITE	22

#define	YALNIX_REGISTER		31
#define YALNIX_SEND		32
#define YALNIX_RECEIVE		33
#define YALNIX_RECEIVESPECIFIC	34
#define YALNIX_REPLY		35
#define YALNIX_FORWARD		36
#define YALNIX_COPY_FROM	37
#define YALNIX_COPY_TO		38

#define YALNIX_READ_SECTOR	41
#define YALNIX_WRITE_SECTOR	42
#else 
//-----------Linux version------------------------------------

#define YALNIX_PREFIX            0xabcdef00    
#define YALNIX_MASK              0x000000FF

#define	YALNIX_FORK		( 0x1 | YALNIX_PREFIX)
#define	YALNIX_EXEC		( 0x2 | YALNIX_PREFIX)
#define	YALNIX_EXIT		( 0x3 | YALNIX_PREFIX)
#define	YALNIX_WAIT		( 0x4 | YALNIX_PREFIX)
#define YALNIX_GETPID           ( 0x5 | YALNIX_PREFIX)
#define	YALNIX_BRK		( 0x6 | YALNIX_PREFIX)
#define	YALNIX_DELAY		( 0x7 | YALNIX_PREFIX)

#define	YALNIX_TTY_READ		( 0x21 | YALNIX_PREFIX)
#define	YALNIX_TTY_WRITE	( 0x22 | YALNIX_PREFIX)

#define	YALNIX_REGISTER		( 0x31 | YALNIX_PREFIX)
#define YALNIX_SEND		( 0x32 | YALNIX_PREFIX)
#define YALNIX_RECEIVE		( 0x33 | YALNIX_PREFIX)
#define YALNIX_RECEIVESPECIFIC	( 0x34 | YALNIX_PREFIX)
#define YALNIX_REPLY		( 0x35 | YALNIX_PREFIX)
#define YALNIX_FORWARD		( 0x36 | YALNIX_PREFIX)
#define YALNIX_COPY_FROM	( 0x37 | YALNIX_PREFIX)
#define YALNIX_COPY_TO		( 0x38 | YALNIX_PREFIX)

#define YALNIX_READ_SECTOR	( 0x41 | YALNIX_PREFIX)
#define YALNIX_WRITE_SECTOR	( 0x42 | YALNIX_PREFIX)

#define YALNIX_PIPE_INIT        ( 0x48 | YALNIX_PREFIX)
#define YALNIX_PIPE_READ        ( 0x49 | YALNIX_PREFIX)
#define YALNIX_PIPE_WRITE       ( 0x4A | YALNIX_PREFIX)

#define YALNIX_NOP	        ( 0x50 | YALNIX_PREFIX)

#define YALNIX_SEM_INIT         ( 0x60 | YALNIX_PREFIX)
#define YALNIX_SEM_UP           ( 0x61 | YALNIX_PREFIX)
#define YALNIX_SEM_DOWN         ( 0x62 | YALNIX_PREFIX)
#define YALNIX_LOCK_INIT        ( 0x63 | YALNIX_PREFIX)
#define YALNIX_LOCK_ACQUIRE     ( 0x64 | YALNIX_PREFIX)
#define YALNIX_LOCK_RELEASE     ( 0x65 | YALNIX_PREFIX)
#define YALNIX_CVAR_INIT        ( 0x66 | YALNIX_PREFIX)
#define YALNIX_CVAR_SIGNAL      ( 0x67 | YALNIX_PREFIX)
#define YALNIX_CVAR_BROADCAST   ( 0x68 | YALNIX_PREFIX)
#define YALNIX_CVAR_WAIT        ( 0x69 | YALNIX_PREFIX)
#define YALNIX_RECLAIM          ( 0x6A | YALNIX_PREFIX)

#define YALNIX_CUSTOM_0         ( 0x70 | YALNIX_PREFIX)
#define YALNIX_CUSTOM_1         ( 0x71 | YALNIX_PREFIX)
#define YALNIX_CUSTOM_2         ( 0x72 | YALNIX_PREFIX)

#define YALNIX_BOOT             ( 0xFF | YALNIX_PREFIX)
#endif

/*
 *  All Yalnix kernel calls return ERROR in case of any error.
 */
#define	ERROR			(-1)

/*
 * Server index definitions for Register(index) and Send(msg, -index):
 */
#define	MAX_SERVER_INDEX	16	/* max legal index */

/*
 * Define the (constant) size of a message for Send/Receive/Reply.
 */
#define	MESSAGE_SIZE		32

#ifndef	__ASM__

#include <sys/types.h>

#ifndef _PARAMS
#if defined(__STDC__) || defined(__cplusplus)
#define _PARAMS(ARGS) ARGS
#else
#define _PARAMS(ARGS) ()
#endif
#endif /* _PARAMS */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Function prototypes for each of the Yalnix kernel calls.
 */

extern int Nop _PARAMS((int,int,int,int));

extern int Fork _PARAMS((void));
extern int Exec _PARAMS((char *, char **));
extern void Exit _PARAMS((int));
extern int Wait _PARAMS((int *));
extern int GetPid _PARAMS((void));
extern int Brk _PARAMS((void *));
extern int Delay _PARAMS((int));
extern int TtyRead _PARAMS((int, void *, int));
extern int TtyWrite _PARAMS((int, void *, int));
extern int Register _PARAMS((unsigned int));
extern int Send _PARAMS((void *, int));
extern int Receive _PARAMS((void *));
extern int ReceiveSpecific _PARAMS((void *, int));
extern int Reply _PARAMS((void *, int));
extern int Forward _PARAMS((void *, int, int));
extern int CopyFrom _PARAMS((int, void *, void *, int));
extern int CopyTo _PARAMS((int, void *, void *, int));
extern int ReadSector _PARAMS((int, void *));
extern int WriteSector _PARAMS((int, void *));

extern int PipeInit _PARAMS((int *));
extern int PipeRead _PARAMS((int, void *, int));
extern int PipeWrite _PARAMS((int, void *, int));

extern int SemInit _PARAMS((int *, int));
extern int SemUp _PARAMS((int));
extern int SemDown _PARAMS((int));
extern int LockInit _PARAMS((int *));
extern int Acquire _PARAMS((int));
extern int Release _PARAMS((int));
extern int CvarInit _PARAMS((int *));
extern int CvarWait _PARAMS((int, int));
extern int CvarSignal _PARAMS((int));
extern int CvarBroadcast _PARAMS((int));

extern int Reclaim _PARAMS((int));

extern int Custom0 _PARAMS((int,int,int,int));
extern int Custom1 _PARAMS((int,int,int,int));
extern int Custom2 _PARAMS((int,int,int,int));

/*
 * A Yalnix library function: TtyPrintf(num, format, args) works like
 * printf(format, args) on terminal num.
 */
extern int TtyPrintf _PARAMS((int, char *, ...));

#ifdef __cplusplus
}
#endif

#endif

#endif /*!_yalnix_h*/
