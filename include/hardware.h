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
 * This file:  External definitions for the hardware on which Yalnix runs.
 *
 * The port involved adjusting for x86 registers and context.
 *
 * Oct 2003: minor change---sws
 *
 * Oct 2013: added TRAP_MEMORY codes---sws
 */

#ifndef _hardware_h
#define _hardware_h


#ifdef LINUX

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for strsignal
#endif

#ifndef __USE_GNU
#define  __USE_GNU   // for the register constants
#endif

#else
#warning "LINUX not defined!---are you sure you meant to do this?"
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <ucontext.h>


// Over the years, yalnix has been run on host systems with varying
// page sizes. Here, we define standard sizes for the Yalnix platform
// ...overriding previous definitions, if necessary.

// The critical thing is that the Yalnix page size be a multiple
// of the host system page size.  In Linux/86, this appears to be 0x1000

#undef PAGESIZE
#define PAGESIZE	0x2000		// 8K
#undef PAGEOFFSET
#define PAGEOFFSET	(PAGESIZE-1)
#undef PAGEMASK
#define PAGEMASK	(~PAGEOFFSET)
#define PAGESHIFT	13

#ifdef LINUX
#if (PAGESIZE != 0x2000)
#warning "pagesize has changed: change alignment in kernel.x, user.x"
#endif
#endif
// note for the future:
// makefile cleverness could REWRITE kernel.x, user.x automatically


/*
 * Define the virtual memory layout of the machine.
 *
 * The virtual memory layout is determined by the design specifications
 * of the machine's architecture.
 */



#define VMEM_REGION_SIZE        0x100000        /* 1 megabyte */

#define	VMEM_NUM_REGION		2
#define	VMEM_SIZE		(VMEM_NUM_REGION * VMEM_REGION_SIZE)

#define	VMEM_BASE		0
#define	VMEM_0_BASE		VMEM_BASE
#define	VMEM_0_SIZE		VMEM_REGION_SIZE
#define	VMEM_0_LIMIT		(VMEM_0_BASE + VMEM_0_SIZE)
#define	VMEM_1_BASE		VMEM_0_LIMIT
#define	VMEM_1_SIZE		VMEM_REGION_SIZE
#define	VMEM_1_LIMIT		(VMEM_1_BASE + VMEM_1_SIZE)
#define	VMEM_LIMIT		VMEM_1_LIMIT



#ifdef LINUX
#if (VMEM_REGION_SIZE != 0x100000)
#warning "memory size has changed: change addresses in user.x"
#endif
#endif
// note for the future:
// makefile cleverness could REWRITE kernel.x, user.x automatically

/* The minimum legal virtual page number for the machine */
#define MIN_VPN			0

/* The number of virtual pages on this machine */
#define NUM_VPN			(VMEM_LIMIT >> PAGESHIFT)

/* The maximum legal virtual page number for the machine */
#define	MAX_VPN			(NUM_VPN - 1)

/* The size of a page table (number of page table entries) to map an
 * entire region of virtual memory */
#define	MAX_PT_LEN		(VMEM_REGION_SIZE >> PAGESHIFT)

/*
 * Define the physical memory layout of the machine.
 *
 * The PMEM_BASE is determined by the machine's architecture design
 * specification.  The actual size of physical memory, though, is
 * determined by how much RAM is installed in the machine.  This
 * is computed by the boot ROM and is passed to your KernelStart
 * at boot time.
 */
#define	PMEM_BASE		0

/* This is where the kernel stack goes.
 * The kernel stack gets instantiated for each trap/interrupt/exception,
 * at this address every time.
 */
/* KERNEL_STACK must be within region 0 of memory */
#define KERNEL_STACK_LIMIT	VMEM_0_LIMIT
#define KERNEL_STACK_MAXSIZE	(2*PAGESIZE)
#define KERNEL_STACK_BASE \
	DOWN_TO_PAGE(KERNEL_STACK_LIMIT - KERNEL_STACK_MAXSIZE)

/*
 * Define the structure of a page table entry.
 */
struct pte {
	u_long valid   : 1;	/* page mapping is valid */
	u_long prot    : 3;	/* page protection bits */
	u_long         : 4;	/* reserved; currently unused */
	u_long pfn     : 24;	/* page frame number */
};

/*
 * Define the protection bits used in page table entries.
 *
 * PROT_READ, PROT_WRITE, and PROT_EXEC come from /usr/include/sys/mman.h
 */
#ifndef	PROT_NONE
#define	PROT_NONE	0
#endif
#ifndef	PROT_ALL
#define	PROT_ALL	(PROT_READ|PROT_WRITE|PROT_EXEC)
#endif

/*
 * The machine has a "translation lookaside buffer" (TLB), which must
 * be flushed during a context switch or when changing page tables.
 * The TLB is flushed by writing one of the following values into
 * the REG_TLB_FLUSH privileged machine register:
 *
 *   - TLB_FLUSH_ALL: flushes the entire TLB.
 *   - TLB_FLUSH_0: flushes all mappings for region 0 from the TLB.
 *   - TLB_FLUSH_1: flushes all mappings for region 1 from the TLB.
 *   - addr: flushes only the mapping for virtual address "addr".
 */
#define	TLB_FLUSH_ALL	(-1)
#define	TLB_FLUSH_0	(-2)
#define	TLB_FLUSH_1	(-3)

/*
 *  Macros for rounding numbers up to a multiple of the page size,
 *  or down to a multiple of the page size.  For example, if PAGESIZE
 *  is 0x1000, then:
 *
 *        addr          UP_TO_PAGE(addr)        DOWN_TO_PAGE(addr)
 *      -------         ----------------        ------------------
 *         0x0                 0x0                       0x0
 *         0x1              0x1000                       0x0
 *       0xfff              0x1000                       0x0
 *      0x1000              0x1000                    0x1000
 *      0x1001              0x2000                    0x1000
 */
#define	UP_TO_PAGE(n)	(((long)(n) + PAGEOFFSET) & PAGEMASK)
#define	DOWN_TO_PAGE(n)	((long)(n) & PAGEMASK)

/*
 *  Amount of room to leave on stack for initial stack frame -- this
 *  is the basic SPARC stack frame size, required by Solaris and the
 *  SPARC architecture.
 */
#ifndef LINUX
#define	INITIAL_STACK_FRAME_SIZE	0x40
#else
#define	INITIAL_STACK_FRAME_SIZE	0
#endif

// Amount of zero'd room to leave on stack after the two NULL pointes
// after the argv array.  In Linux/glibc, __libc_start_main casts
// the first int after the NULL after argv to an elf structure, and
// starts doing a search from there....   so we want at least one whole
// elf structure's worth of nulls.

#ifdef LINUX
#define POST_ARGV_NULL_SPACE          0x20
#else
#define POST_ARGV_NULL_SPACE          0
#endif




#define GREGS  8          // how many gp regs in the original yalnix machine


#ifdef LINUX
#define REAL_REG_COUNT 6  // how many gp regs in the x86
#define GREG_EAX 0
#define GREG_EBX 1
#define GREG_ECX 2
#define GREG_EDX 3
#define GREG_ESI 4
#define GREG_EDI 5

#ifndef REG_EAX   // some versions of gcc/linux use the old names...
  #define REG_EAX EAX
  #define REG_EBX EBX
  #define REG_ECX ECX
  #define REG_EDX EDX
  #define REG_ESI ESI
  #define REG_EDI EDI
  #define REG_EIP EIP
  #define REG_ESP ESP
  #define REG_ERR ERR
  #define REG_TRAPNO TRAPNO
  #define REG_EBP EBP
#endif

extern const int y_to_x86[REAL_REG_COUNT];  // to translate registers
// ... since Linux does not have EAX, EBX,... in any nice order
// in the ucontext_t
#endif



/*
 * Define the user level context, pushed onto the kernel stack by
 * the hardware when any interrupt, trap, or exception occurs.  The
 * address of the user context is passed to the handler function
 * called through the interrupt vector.  An initial user context
 * is also built by the boot ROM and is passed to KernelStart.
 */
struct user_context {
  int vector;		/* vector number */
  int code;		/* additional "code" for vector */
  void *addr;		/* offending address, if any */
  void *pc;		/* PC at time of exception */
  void *sp;		/* SP at time of exception */
#ifdef LINUX
  void *ebp;              // base pointer at time of exception
#endif

  u_long regs[GREGS];     /* general registers at time of exception */
};

typedef struct user_context UserContext;

/*
 * This is the structure that holds a current context of the CPU.
 * Used for the kernel context, since the user context is provided
 * elsewhere, and is not active while the kernel is running.
 */
typedef ucontext_t KernelContext;

/*
 *  Define the interrupt and exception vector numbers.  These numbers
 *  are used as a subscript into the vector table, which is pointed to
 *  by the hardware register REG_VECTOR_BASE.  The vector table must be
 *  built as TRAP_VECTOR_SIZE entries long, although not all entries
 *  are currently used by the hardware.  All unused entries should be
 *  initialized to NULL.
 */
#define	TRAP_KERNEL		0
#define	TRAP_CLOCK		1
#define	TRAP_ILLEGAL		2
#define	TRAP_MEMORY		3
#define	TRAP_MATH		4
#define	TRAP_TTY_RECEIVE	5
#define	TRAP_TTY_TRANSMIT	6
#define	TRAP_DISK		7

#define	TRAP_VECTOR_SIZE	16	/* dimensioned size of array */


/*
 * alleged TRAP_MEMORY codes (gleaned from examination of Linux host behavior)
 */

#define YALNIX_MAPERR 1
#define YALNIX_ACCERR 0


/*
 *  Definitions for the hardware register names.  These names
 *  are used with the WriteRegister and ReadRegister operations.
 */
#define REG_VECTOR_BASE	1	/* vector base */
#define REG_VM_ENABLE	2	/* virtual memory enable */
#define REG_TLB_FLUSH	3	/* translation lookaside buffer flush */
#define REG_PTBR0	4	/* inv page table base for region 0 */
#define REG_PTLR0	5	/* inv page table limit for region 0 */
#define REG_PTBR1	6	/* page table base for region 1 */
#define REG_PTLR1	7	/* page table limit for region 1 */

/*
 *  Definitions for the terminals attached to the machine.
 *
 *  The first terminal is the system console, but there is no difference
 *  in the hardware between it and the other terminals.  The difference
 *  is only in the use that Yalnix user processes make of the terminals.
 */

#define	TTY_CONSOLE	0		/* terminal 0 is the console */
#define	TTY_1		1
#define	TTY_2		2
#define	TTY_3		3

#define	NUM_TERMINALS	4		/* # of terminals, including console */

#define	TERMINAL_MAX_LINE	1024	/* maximum length of terminal line */

/*
 * Some function definitions.
 */

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
 * Definitions for the special operations provided by the hardware.
 */

extern int TtyReceive _PARAMS((int, void *, int));
extern void TtyTransmit _PARAMS((int, void *, int));
extern void Halt _PARAMS((void));
extern void WriteRegister _PARAMS((int, unsigned int));
extern unsigned int ReadRegister _PARAMS((int));
extern void Pause _PARAMS((void));
extern void TracePrintf _PARAMS((int, char *, ...));
extern void DiskAccess _PARAMS((int, int, void *));

/*
 * Definitions of functions to be written by student
 */

extern int SetKernelBrk _PARAMS((void *));

/* This is the primary entry point into the kernel */
extern void KernelStart _PARAMS((char**, unsigned int, UserContext *));

extern void SetKernelData _PARAMS((void*,void*));

/*
 * Switching kernel context while inside the kernel is tricky.  Real
 * operating systems do this within one function that is careful not
 * to use local variables or make function calls, so that the kernel
 * stack can be switched without messing up the switch itself.  The
 * operating system also has to be careful to save and restore registers
 * without interfering with the code that saves and restores them.
 * For these reasons, this is usually done in carefully-written
 * assembly.
 *
 * To make the task of switching kernel context easier in the project,
 * we provide the function:
 *
 *     int KernelContextSwitch(KCSFunc_t *, void *, void *)
 *
 * The type KCSFunc_t (kernel context switch function type) is a messy
 * C typedef of a special kind of function (matched by MyKCS, for
 * example, below).
 *
 * KernelContextSwitch temporarily stops using the standard kernel
 * context and calls a function provided by you (suppose your function
 * is called MyKCS):
 *
 *     KernelContext *MyKCS(KernelContext *, void *, void *)
 *
 * The two "void *" arguments to KernelContextSwitch will be passed
 * unmodified to MyKCS.  You should use them to point to the
 * current process's PCB and to the PCB of the new process to be
 * context switched in, respectively.  In MyKCS, you should do
 * whatever you need to do to switch kernel contexts between these
 * two processes.  The "KernelContext *" passed to MyKCS is a
 * pointer to a temporary copy of the current kernel context somewhere
 * in memory, which you should copy into the old process's PCB.  The
 * "KernelContext *" returned by MyKCS should point to a copy of
 * the kernel context of the new process to run.
 *
 * You should use KernelContextSwitch to "block" a process somewhere
 * in the middle of some procedure somewhere inside your kernel.
 * KernelContextSwitch and MyKCS do the context switch, and when
 * this process is later context switched back to, KernelContextSwitch
 * returns.  The return value of KernelContextSwitch in this case is 0.
 * If, instead, any error occurs, KernelContextSwitch does not switch
 * contexts and returns -1.
 */
typedef KernelContext *KCSFunc_t(KernelContext *, void *, void *);
extern int KernelContextSwitch(KCSFunc_t *, void *, void *);

/*
 *  Define the physical properties of the disk.
 */
#define	SECTORSIZE	512	/* size of a disk sector in bytes */
#define	NUMSECTORS	1426	/* number of sectors on the disk */

/*
 *  Define the operation codes for DiskAccess, the hardware operation
 *  that the kernel uses to read/write sectors on the disk.
 */
#define DISK_READ	0
#define DISK_WRITE	1

#ifdef __cplusplus
}
#endif

#endif /*!_hardware_h*/

