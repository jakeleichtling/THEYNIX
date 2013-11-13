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
USER_APPS = init idle die_stupidly orphan io_test pipe_test lock_test TestDriver LedyardBridge
#List all user program source files here.  SHould be the same as the previous list, with ".c" added to each file
USER_SRCS = init.c idle.c die_stupidly.c orphan.c io_test.c pipe_test.c lock_test.c TestDriver.c LedyardBridge.c
#List the objects to be formed form the user  source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
USER_OBJS = init.o idle.o die_stupidly.o orphan.o io_test.o pipe_test.o lock_test.o TestDriver.o LedyardBridge.o
#List all of the header files necessary for your user programs
USER_INCS = Log.h

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
	rm -f *.o *~ TTYLOG* TRACE $(YALNIX_OUTPUT) $(USER_APPS)  core.* TestDriver

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

test:
	gcc -g -Wall -o TestDriver List.c List.h TestDriver.c

$(KERNEL_ALL): $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o $@ $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

$(USER_APPS): $(USER_OBJS) $(USER_INCS)
	$(ETCDIR)/yuserbuild.sh $@ $(DDIR58) $@.o










