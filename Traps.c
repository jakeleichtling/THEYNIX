#include "Traps.h"

#include "Kernel.h"
#include "Log.h"
#include "PCB.h"
#include "include/hardware.h"

// Assume stack is growing down if within
// this many pages of current stack bottom.
#define MAX_STACK_PAGE_JUMP 1

void TrapKernel() {}

void TrapClock() {}

void TrapIllegal() {}

void TrapMemory() {
    int code = current_proc->user_context->code;
    void *addr = current_proc->user_context->addr;

    if (YALNIX_MAPERR == code) { // "address not mapped"
        unsigned int addr_page = ADDR_TO_PAGE(addr);
        if (addr_page >= (current_proc->lowest_user_stack_page - MAX_STACK_PAGE_JUMP)
            && addr_page < current_proc->lowest_user_stack_page) {
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Growing User stack\n");
            // TODO: enlarge user stack!
        } else {
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Out of range memory access at %p.\n", addr);
            KillCurrentProc();
        }
    } else if {YALNIX_ACCERR == code) { // "invalid permissions"
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
    int *table = malloc(sizeof(int) * TRAP_VECTOR_SIZE);

    // Initialize all valid trap vector entries
    for (int *t = table, i = 0; i < TRAP_VECTOR_SIZE; t++, i++) {
        t[i] = &TrapNotDefined;
    }

    table[TRAP_KERNEL] = &TrapKernel;
    table[TRAP_CLOCK] = &TrapClock;
    table[TRAP_MEMORY] = &TrapMemory;
    table[TRAP_MATH] = &TrapMath;
    table[TRAP_TTY_RECIEVE] = &TrapTtyRecieve;
    table[TRAP_TTY_TRANSMIT] = &TrapTtyTransmit;

    WriteRegister(REG_VECTOR_BASE, (unsigned int) table);
}

