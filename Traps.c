#include "Traps.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <yalnix.h>
#include <hardware.h>
#include <stdio.h>

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
        case YALNIX_EXEC:
            rc = KernelExec((char *) user_context->regs[0],
                (char **) user_context->regs[1], user_context);
            break;
        case YALNIX_WAIT:
            rc = KernelWait((int *)user_context->regs[0], user_context);
            break;
        case YALNIX_EXIT:
            KernelExit(user_context->regs[0], user_context);
            break;
        case YALNIX_BRK:
            rc = KernelBrk((void *) user_context->regs[0]);
            break;
        case YALNIX_TTY_READ:
            rc = KernelTtyRead(user_context->regs[0], (void *) user_context->regs[1],
                 user_context->regs[2], user_context);
            break;
        case YALNIX_TTY_WRITE:
            rc = KernelTtyWriteInternal(user_context->regs[0], (void *) user_context->regs[1],
                 user_context->regs[2], user_context);
            break;
        case YALNIX_PIPE_INIT:
            rc = KernelPipeInit((int *) user_context->regs[0]);
            break;
        case YALNIX_PIPE_READ:
            rc = KernelPipeRead(user_context->regs[0], (void *) user_context->regs[1],
                user_context->regs[2], user_context);
            break;
        case YALNIX_PIPE_WRITE:
            rc = KernelPipeWrite(user_context->regs[0], (void *) user_context->regs[1],
                user_context->regs[2], user_context);
            break;
        case YALNIX_LOCK_INIT:
            rc = KernelLockInit((int *) user_context->regs[0]);
            break;
        case YALNIX_LOCK_ACQUIRE:
            rc = KernelAcquire(user_context->regs[0], user_context);
            break;
        case YALNIX_LOCK_RELEASE:
            rc = KernelRelease(user_context->regs[0]);
            break;
        case YALNIX_CVAR_INIT:
            rc = KernelCvarInit((int *) user_context->regs[0]);
            break;
        case YALNIX_CVAR_SIGNAL:
            rc = KernelCvarSignal(user_context->regs[0]);
            break;
        case YALNIX_CVAR_BROADCAST:
            rc = KernelCvarBroadcast(user_context->regs[0]);
            break;
        case YALNIX_CVAR_WAIT:
            rc = KernelCvarWait(user_context->regs[0], user_context->regs[1], user_context);
            break;
        case YALNIX_RECLAIM:
            rc = KernelReclaim(user_context->regs[0]);
            break;
        default:
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "TrapKernel: Code %d undefined\n");
            KernelExit(KILLED_KERNEL_TRAP_NOT_DEFINED, user_context);
            rc = THEYNIX_EXIT_FAILURE;
            break;
    }
    user_context->regs[0] = rc;
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapKernel() rc=%d\n", rc);
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

    if (current_proc->pid != IDLE_PID) {
        ListAppend(ready_queue, current_proc, current_proc->pid);
    }

    SwitchToNextProc(user_context);

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapClock(%p)\n", user_context);
}

void TrapIllegal(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapIllegal(%p)\n", user_context);
    KernelExit(KILLED_TRAP_ILLEGAL, user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapIllegal(%p)\n", user_context);
}

void TrapMemory(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapMemory(%p)\n", user_context);

    unsigned int addr_int = (unsigned int) user_context->addr;

    if (addr_int > VMEM_1_LIMIT || addr_int < VMEM_1_BASE) {
        char *err_str = calloc(TERMINAL_MAX_LINE, sizeof(char)); 
        sprintf(err_str, "Out of range memory access at %x by proc %d\n",
            user_context->addr, current_proc->pid);
        KernelTtyWriteInternal(0, err_str, strnlen(err_str, TERMINAL_MAX_LINE), user_context);
        free(err_str);
        KernelExit(THEYNIX_EXIT_FAILURE, user_context);
    }
    int addr_page = ADDR_TO_PAGE(user_context->addr - VMEM_1_BASE);
    if (current_proc->region_1_page_table[addr_page].valid != 1) { // "address not mapped"

        bool below_current_stack = (addr_page < current_proc->lowest_user_stack_page);
        bool above_heap = (addr_page > current_proc->user_brk_page);
        if (below_current_stack && above_heap) { // valid stack growth
            TracePrintf(TRACE_LEVEL_DETAIL_INFO, "Growing User stack\n");
            
            unsigned int page_to_alloc = current_proc->lowest_user_stack_page - 1;
            while (page_to_alloc >= addr_page) {
                int new_frame = GetUnusedFrame(unused_frames);
                if (new_frame == THEYNIX_EXIT_FAILURE) {
                    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "GetUnusedFrame() failed.\n");

                    char *err_str = calloc(TERMINAL_MAX_LINE, sizeof(char)); 
                    sprintf(err_str, "Proc %d tried to grow stack, but out of free frames\n",
                        current_proc->pid);
                    KernelTtyWriteInternal(0, err_str, strnlen(err_str, TERMINAL_MAX_LINE),
                         user_context);
                    free(err_str);

                    KernelExit(THEYNIX_EXIT_FAILURE, user_context);
                }
                assert(!current_proc->region_1_page_table[page_to_alloc].valid);

                current_proc->region_1_page_table[page_to_alloc].valid = 1;
                current_proc->region_1_page_table[page_to_alloc].pfn = new_frame;
                current_proc->region_1_page_table[page_to_alloc].prot = PROT_READ | PROT_WRITE;

                --page_to_alloc;
            }

            current_proc->lowest_user_stack_page = addr_page;
        } else if (!above_heap) { // OOM!
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM,
                "Out of mem on stack growth at %p\n", user_context->addr);
            char *err_str = calloc(TERMINAL_MAX_LINE, sizeof(char)); 
            sprintf(err_str, "Proc %d tried to grow stack, but out of free frames\n",
                current_proc->pid);
            KernelTtyWriteInternal(0, err_str, strnlen(err_str, TERMINAL_MAX_LINE), user_context);
            free(err_str);
            KernelExit(THEYNIX_EXIT_FAILURE, user_context);
        } else { // not below the user stack? should not happen!
            TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Somehow unmapped addr is above the bottom of the stack\n");
            char *err_str = calloc(TERMINAL_MAX_LINE, sizeof(char)); 
            sprintf(err_str, "Proc %d found an unmapped page in its stack. Sorry.\n",
                current_proc->pid);
            KernelTtyWriteInternal(0, err_str, strnlen(err_str, TERMINAL_MAX_LINE), user_context);
            free(err_str);
            KernelExit(THEYNIX_EXIT_FAILURE, user_context);
        }
    } else { // Page was mapped and in range, so must be invalid permissions
        TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Proc %d accessed mem with invalid permissions\n", current_proc->pid);
        char *err_str = calloc(TERMINAL_MAX_LINE, sizeof(char)); 
        sprintf(err_str, "Proc %d accessed %x with invalid permissions\n",
            current_proc->pid, user_context->addr);
        KernelTtyWriteInternal(0, err_str, strnlen(err_str, TERMINAL_MAX_LINE), user_context);
        free(err_str);
        exit(-1);
    }
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapMemory()\n\n");
}

void TrapMath(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapMath(%p)\n", user_context);
    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Killing proc on trap math \n");
    KernelExit(KILLED_TRAP_MATH, user_context);
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapMath(%p)\n", user_context);
}

void TrapTtyRecieve(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapTtyRecieve(%p)\n", user_context);
    int tty_id = user_context->code;
    Tty term = ttys[tty_id];
    if (ListEmpty(term.waiting_to_receive)) { // no waiting procs
        LineBuffer *lb = calloc(1, sizeof(LineBuffer));
        lb->buffer = calloc(TERMINAL_MAX_LINE, sizeof(char));
        lb->length = TtyReceive(tty_id, lb->buffer, TERMINAL_MAX_LINE);
        ListEnqueue(term.line_buffers, lb, 0);
    } else { // at least one proc waiting
        PCB *waiting_proc = (PCB *) ListDequeue(term.waiting_to_receive);
        assert(waiting_proc->tty_receive_buffer);

        ListAppend(ready_queue, waiting_proc, waiting_proc->pid);
        char *input = calloc(TERMINAL_MAX_LINE, sizeof(char));
        int input_length = TtyReceive(tty_id, input, TERMINAL_MAX_LINE);
        if (input_length <= waiting_proc->tty_receive_len) {
            strncpy(waiting_proc->tty_receive_buffer, input, input_length);
        } else {
            strncpy(waiting_proc->tty_receive_buffer, input, waiting_proc->tty_receive_len);
            int remaining_length = input_length - waiting_proc->tty_receive_len;
            char *remaining = calloc(remaining_length, sizeof(char));
            strncpy(remaining, input + waiting_proc->tty_receive_len, remaining_length);
            LineBuffer *lb = calloc(1, sizeof(LineBuffer));
            lb->buffer = input;
            lb->length = remaining_length;
            ListEnqueue(term.line_buffers, lb, 0);
        }
        free(input);
    }
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapTtyRecieve(%p)\n", user_context);
}

void TrapTtyTransmit(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, ">>> TrapTtyTransmit(%p)\n", user_context);
    int tty_id = user_context->code;
    Tty term = ttys[tty_id];
    assert(!ListEmpty(term.waiting_to_transmit));

    // Get the currently transmitting proc
    PCB *waiting_proc = (PCB *) ListPeak(term.waiting_to_transmit);
    if (waiting_proc->tty_transmit_len > TERMINAL_MAX_LINE) { // not completely transmitted
        waiting_proc->tty_transmit_pointer += TERMINAL_MAX_LINE;
        waiting_proc->tty_transmit_len -= TERMINAL_MAX_LINE;

        if (TERMINAL_MAX_LINE > waiting_proc->tty_transmit_len) {
            TtyTransmit(tty_id, waiting_proc->tty_transmit_pointer,
                waiting_proc->tty_transmit_len);
        } else {
            TtyTransmit(tty_id, waiting_proc->tty_transmit_pointer,
                TERMINAL_MAX_LINE);
        }
    } else { // transmission complete
        // since done, take off transmitting list
        ListRemoveById(term.waiting_to_transmit, waiting_proc->pid);
        ListAppend(ready_queue, waiting_proc, waiting_proc->pid);
        free(waiting_proc->tty_transmit_buffer);
    }

    if (ListEmpty(term.waiting_to_transmit)) {
        return; // no other procs waiting on this term
    }

    // Get the next proc waiting to submit
    PCB *next_to_transmit = (PCB *) ListPeak(term.waiting_to_transmit);
    if (TERMINAL_MAX_LINE > next_to_transmit->tty_transmit_len) {
        TtyTransmit(tty_id, next_to_transmit->tty_transmit_pointer,
            next_to_transmit->tty_transmit_len);
    } else {
        TtyTransmit(tty_id, next_to_transmit->tty_transmit_pointer,
            TERMINAL_MAX_LINE);
    }

    TracePrintf(TRACE_LEVEL_FUNCTION_INFO, "<<< TrapTtyTransmit(%p)\n", user_context);
}

void TrapNotDefined(UserContext *user_context) {
    TracePrintf(TRACE_LEVEL_NON_TERMINAL_PROBLEM, "Unknown TRAP call. Killing proc\n");
    KernelExit(KILLED_KERNEL_TRAP_NOT_DEFINED, user_context);

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

