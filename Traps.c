#include "Traps.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <yalnix.h>
#include <hardware.h>

#include "Kernel.h"
#include "Log.h"
#include "PCB.h"
#include "SystemCalls.h"

extern List *clock_block_procs;
extern List *ready_queue;
extern PCB *current_proc;

void TrapKernel(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapKernel(%p)\n", user_context);
    int rc;
    switch(user_context->code){
        case YALNIX_DELAY:
            rc = KernelDelay(user_context->regs[0], user_context);
            break;
        case YALNIX_FORK:
            rc = KernelFork(user_context);
            break;
        case YALNIX_GETPID:
            rc = KernelGetPid();
            break;
        default:
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "TrapKernel: Code %d undefined\n");
            rc = THEYNIX_EXIT_FAILURE;
            break;
    }
    user_context->regs[0] = rc;
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapKernel(%p)\n", user_context);
}

void DecrementTicksRemaining(void *_proc) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> DecrementTicksRemaining(%p)\n", _proc);
    PCB *proc = (PCB *) _proc;
    --proc->clock_ticks_until_ready;
    if (proc->clock_ticks_until_ready <= 0) {
        TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> DecrementTicksRemaining: proc %p done waiting!\n",
             _proc);

        ListRemoveById(clock_block_procs, proc->pid);
        ListAppend(ready_queue, proc, proc->pid);
    }
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< DecrementTicksRemaining()\n");
}

void TrapClock(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapClock(%p)\n", user_context);
    ListMap(clock_block_procs, &DecrementTicksRemaining);

    // place current proc in end of the ready queue
    ListAppend(ready_queue, current_proc, current_proc->pid);

    SwitchToNextProc(user_context);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapClock(%p)\n", user_context);
}

void TrapIllegal(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapIllegal(%p)\n", user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapIllegal(%p)\n", user_context);
}

inline bool ValidStackGrowth(const unsigned int page) {
    bool below_current_stack = (page < current_proc->lowest_user_stack_page);
    bool above_heap = (page > current_proc->user_brk_page);
    return below_current_stack && above_heap;
}

void TrapMemory(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapMemory(%p)\n", user_context);

    if (YALNIX_MAPERR == user_context->code) { // "address not mapped"
        if (((unsigned int) user_context->addr) < VMEM_1_BASE) {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "User program tried to address kernel space @ %p\n", user_context->addr);
            //TODO: kill program rather than die
            exit(-1);
        }
        unsigned int addr_page = ADDR_TO_PAGE(user_context->addr) - ADDR_TO_PAGE(VMEM_1_BASE);
        if (ValidStackGrowth(addr_page)) {
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Growing User stack\n");

            unsigned int page_to_alloc = current_proc->lowest_user_stack_page - 1;
            while (page_to_alloc >= addr_page) {
                int new_frame = GetUnusedFrame(unused_frames);
                if (new_frame == THEYNIX_EXIT_FAILURE) {
                    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "GetUnusedFrame() failed.\n");
                    KillCurrentProc();
                }
                assert(!current_proc->region_1_page_table[page_to_alloc].valid);

                current_proc->region_1_page_table[page_to_alloc].valid = 1;
                current_proc->region_1_page_table[page_to_alloc].pfn = new_frame;
                current_proc->region_1_page_table[page_to_alloc].prot = PROT_READ | PROT_WRITE;

                --page_to_alloc;
            }

            current_proc->lowest_user_stack_page = addr_page;
        } else {
            if (addr_page <= current_proc->user_brk_page) {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "Out of mem on stack growth at %p\n", user_context->addr);
            } else {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                    "Out of range memory access at %p.\n", user_context->addr);
            }
            KillCurrentProc();
        }
    } else if (YALNIX_ACCERR == user_context->code) { // "invalid permissions"
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
            "Invalid memory permission at %p.\n", user_context->addr);
        KillCurrentProc();
    } else {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM,
            "Unknown TRAP_MEMORY user_context->code %d\n", user_context->code);
        exit(-1);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapMemory()\n\n");
}

void TrapMath(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapMath(%p)\n", user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapMath(%p)\n", user_context);
}

void TrapTtyRecieve(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapTtyRecieve(%p)\n", user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapTtyRecieve(%p)\n", user_context);
}

void TrapTtyTransmit(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapTtyTransmit(%p)\n", user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapTtyTransmit(%p)\n", user_context);
}

void TrapNotDefined(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Unknown TRAP call.\n");
}

void TrapTableInit() {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapTableInit()\n");

    void **table = (void *) calloc(TRAP_VECTOR_SIZE, sizeof(void *));

    // Initialize all valid trap vector entries
    unsigned int i;
    for (i = 0; i < TRAP_VECTOR_SIZE; i++) {
        table[i]  = (void*) &TrapNotDefined;
    }

    table[TRAP_KERNEL] = (void*) &TrapKernel;
    table[TRAP_CLOCK] = (void*) &TrapClock;
    table[TRAP_ILLEGAL] = (void*) &TrapIllegal;
    table[TRAP_MEMORY] = (void*) &TrapMemory;
    table[TRAP_MATH] = (void*) &TrapMath;
    table[TRAP_TTY_RECEIVE] = (void*) &TrapTtyRecieve;
    table[TRAP_TTY_TRANSMIT] = (void*) &TrapTtyTransmit;

    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Trap vector table address: %p\n", table);
    WriteRegister(REG_VECTOR_BASE, (unsigned int) table);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapTableInit()\n");
}

