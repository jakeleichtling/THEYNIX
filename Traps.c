#include "Traps.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Kernel.h"
#include "Log.h"
#include "PCB.h"
#include "include/hardware.h"

void TrapKernel(UserContext *user_context) {}

void TrapClock(UserContext *user_context) {}

void TrapIllegal(UserContext *user_context) {}

inline bool ValidStackGrowth(const unsigned int page) {
    bool below_current_stack = (page < current_proc->lowest_user_stack_page);
    bool above_heap = (page > current_proc->user_brk_page);
    return below_current_stack && above_heap;
}

void TrapMemory(UserContext *user_context) {
    if (YALNIX_MAPERR == user_context->code) { // "address not mapped"
        unsigned int addr_page = ADDR_TO_PAGE(user_context->addr);
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
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Out of mem on stack growth at %p\n", user_context->addr);
            } else {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Out of range memory access at %p.\n", user_context->addr);
            }
            KillCurrentProc();
        }
    } else if (YALNIX_ACCERR == user_context->code) { // "invalid permissions"
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid memory permission at %p.\n", user_context->addr);
        KillCurrentProc();
    } else {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Unknown TRAP_MEMORY user_context->code %d\n", user_context->code);
        exit(-1);
    }
}

void TrapMath(UserContext *user_context) {}

void TrapTtyRecieve(UserContext *user_context) {}

void TrapTtyTransmit(UserContext *user_context) {}

void TrapNotDefined(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Unknown TRAP call.\n");
}

void TrapTableInit() {
    unsigned int *table = malloc(sizeof(unsigned int) * TRAP_VECTOR_SIZE);

    // Initialize all valid trap vector entries
    unsigned int *t;
    int i;
    for (t = table, i = 0; i < TRAP_VECTOR_SIZE; t++, i++) {
        t[i] = (unsigned int) &TrapNotDefined;
    }

    table[TRAP_KERNEL] = (unsigned int) &TrapKernel;
    table[TRAP_CLOCK] = (unsigned int) &TrapClock;
    table[TRAP_MEMORY] = (unsigned int) &TrapMemory;
    table[TRAP_MATH] = (unsigned int) &TrapMath;
    table[TRAP_TTY_RECEIVE] = (unsigned int) &TrapTtyRecieve;
    table[TRAP_TTY_TRANSMIT] = (unsigned int) &TrapTtyTransmit;

    WriteRegister(REG_VECTOR_BASE, (unsigned int) table);
}

