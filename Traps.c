#include "Traps.h"

#include "Kernel.h"
#include "Log.h"
#include "PCB.h"
#include "include/hardware.h"
#include <stdbool.h>
#include <stdlib.h>

void TrapKernel() {}

void TrapClock() {}

void TrapIllegal() {}

inline bool ValidStackGrowth(const unsigned int page) {
    bool below_current_stack = (page < current_proc->lowest_user_stack_page);
    bool above_heap = (page > current_proc->user_brk_page);
    return below_current_stack && above_heap;
}

void TrapMemory() {
    int code = current_proc->user_context->code;
    void *addr = current_proc->user_context->addr;

    if (YALNIX_MAPERR == code) { // "address not mapped"
        unsigned int addr_page = ADDR_TO_PAGE(addr);
        if (ValidStackGrowth(addr_page)) {
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Growing User stack\n");

            unsigned int page_to_alloc = current_proc->lowest_user_stack_page - 1;
            while (page_to_alloc >= addr_page) {
                int new_frame = GetUnusedFrame(unused_frames);
                if (new_frame == EXIT_FAILURE) {
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
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Out of mem on stack growth at %p\n", addr);
            } else {
                TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Out of range memory access at %p.\n", addr);
            }
            KillCurrentProc();
        }
    } else if (YALNIX_ACCERR == code) { // "invalid permissions"
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Invalid memory permission at %p.\n", addr);
        KillCurrentProc();
    } else {
        TracePrintf(TRACE_LEVEL_TERMINAL_PROBLEM, "Unknown TRAP_MEMORY code %d\n", code);
        exit(-1);
    }
}

void TrapMath() {}

void TrapTtyRecieve() {}

void TrapTtyTransmit() {}

void TrapNotDefined() {
    TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Unknown TRAP call.\n");
}

void TrapTableInit() {
    unsigned int *table = malloc(sizeof(unsigned int) * TRAP_VECTOR_SIZE);

    // Initialize all valid trap vector entries
    for (int *t = table, i = 0; i < TRAP_VECTOR_SIZE; t++, i++) {
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

