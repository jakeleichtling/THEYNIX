#
#	Sample Makefile for Yalnix kernel and user programs.
#
#	Prepared by Sean Smith and Adam Salem and various Yalnix developers
#	of years past...
#
#	You must modify the KERNEL_SRCS and KERNEL_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix kernel.
#
#	You must modify the USER_SRCS and USER_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix user programs
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#

#make all will make all the kernel objects and user objects
ALL = $(KERNEL_ALL) $(USER_APPS)
KERNEL_ALL = yalnix

#List all kernel source files here.
KERNEL_SRCS = Kernel.c PCB.c SystemCalls.c Traps.c VMem.c List.c PMem.c Tty.c LoadProgram.c Pipe.c Lock.c CVar.c
#List the objects to be formed form the kernel source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
KERNEL_OBJS = Kernel.o PCB.o SystemCalls.o Traps.o VMem.o List.o PMem.o Tty.o LoadProgram.o Pipe.o Lock.o CVar.o
#List all of the header files necessary for your kernel
KERNEL_INCS = CVar.h Lock.h PMem.h Traps.h Kernel.h Log.h Pipe.h Tty.h List.h PCB.h SystemCalls.h VMem.h

#List all user programs here.
USER_APPS = idle theynix_tests/io_test theynix_tests/pipe_test theynix_tests/lock_test theynix_tests/LedyardTestDriver theynix_tests/LedyardBridge theynix_tests/cvar_test theynix_tests/stack_growth_test cs58_tests/bigstack cs58_tests/forktest cs58_tests/torture cs58_tests/zero theynix_tests/fork_oom_test theynix_tests/bad_exec_test theynix_tests/child_chain theynix_tests/exit_subtleties_test theynix_tests/bad_wait_test theynix_tests/brk_test theynix_tests/delay_test theynix_tests/test_trap_math theynix_tests/reclaim_test
#List all user program source files here.  SHould be the same as the previous list, with ".c" added to each file
USER_SRCS = idle.c theynix_tests/io_test.c theynix_tests/pipe_test.c theynix_tests/lock_test.c theynix_tests/LedyardTestDriver.c theynix_tests/LedyardBridge.c theynix_tests/cvar_test.c theynix_tests/stack_growth_test.c cs58_tests/bigstack.c cs58_tests/forktest.c cs58_tests/torture.c cs58_tests/zero.c theynix_tests/fork_oom_test.c theynix_tests/bad_exec_test.c theynix_tests/child_chain.c theynix_tests/exit_subtleties_test.c theynix_tests/bad_wait_test.c theynix_tests/brk_test.c theynix_tests/delay_test.c theynix_tests/test_trap_math.c theynix_tests/reclaim_test.c

#List the objects to be formed form the user  source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
USER_OBJS = idle.o theynix_tests/io_test.o theynix_tests/pipe_test.o theynix_tests/lock_test.o theynix_tests/LedyardTestDriver.o theynix_tests/LedyardBridge.o theynix_tests/cvar_test.o theynix_tests/stack_growth_test.o cs58_tests/bigstack.o cs58_tests/forktest.o cs58_tests/torture.o cs58_tests/zero.o theynix_tests/fork_oom_test.o theynix_tests/bad_exec_test.o theynix_tests/child_chain.o theynix_tests/exit_subtleties_test.o theynix_tests/bad_wait_test.o theynix_tests/brk_test.o theynix_tests/delay_test.o theynix_tests/test_trap_math.o theynix_tests/reclaim_test.o


#List all of the header files necessary for your user programs
USER_INCS = Log.h theynix_tests/LedyardBridge.h

#write to output program yalnix
YALNIX_OUTPUT = yalnix

#
#	These definitions affect how your kernel is compiled and linked.
#       The kernel requires -DLINUX, to
#	to add something like -g here, that's OK.
#

#Set additional parameters.  Students generally should not have to change this next section

#Use the gcc compiler for compiling and linking
CC = gcc

DDIR58 = /net/class/cs58/yalnix
LIBDIR = $(DDIR58)/lib
INCDIR = $(DDIR58)/include
ETCDIR = $(DDIR58)/etc

# any extra loading flags...
LD_EXTRA =

KERNEL_LIBS = $(LIBDIR)/libkernel.a $(LIBDIR)/libhardware.so

# the "kernel.x" argument tells the loader to use the memory layout
# in the kernel.x file..
KERNEL_LDFLAGS = $(LD_EXTRA) -L$(LIBDIR) -lkernel -lelf -Wl,-T,$(ETCDIR)/kernel.x -Wl,-R$(LIBDIR) -lhardware
LINK_KERNEL = $(LINK.c)

#
#	These definitions affect how your Yalnix user programs are
#	compiled and linked.  Use these flags *only* when linking a
#	Yalnix user program.
#

USER_LIBS = $(LIBDIR)/libuser.a
ASFLAGS = -D__ASM__
CPPFLAGS= -m32 -fno-builtin -I. -I$(INCDIR) -g -DLINUX -Wall


##########################
#Targets for different makes
# all: make all changed components (default)
# clean: remove all output (.o files, temp files, LOG files, TRACE, and yalnix)
# count: count and give info on source files
# list: list all c files and header files in current directory
# kill: close tty windows.  Useful if program crashes without closing tty windows.
# $(KERNEL_ALL): compile and link kernel files
# $(USER_ALL): compile and link user files
# %.o: %.c: rules for setting up dependencies.  Don't use this directly
# %: %.o: rules for setting up dependencies.  Don't use this directly

all: $(ALL)

clean:
	rm -f *.o *~ TTYLOG* TRACE $(YALNIX_OUTPUT) $(USER_APPS)  core.* theynix_tests/*.o cs58_tests/*.o

new:
	make clean
	make

git:
	git pull
	make new

count:
	wc $(KERNEL_SRCS) $(USER_SRCS)

list:
	ls -l *.c *.h

kill:
	killall yalnixtty yalnixnet yalnix

no-core:
	rm -f core.*

$(KERNEL_ALL): $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o $@ $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

$(USER_APPS): $(USER_OBJS) $(USER_INCS)
	$(ETCDIR)/yuserbuild.sh $@ $(DDIR58) $@.o










