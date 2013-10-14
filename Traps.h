#ifndef _TRAPS_H_
#define _TRAPS_H_

/*

TRAP_KERNEL:
Find the appropriate syscall and call it. Put the return value into reg 0.

TRAP_CLOCK:
Iterate through clock_blocked procs and decrement clock_ticks_until_ready. If decremented to 0,
remove from clock_blocked and add to ready queue. Context switch!

TRAP_ILLEGAL:
Print trace. Call exit on process with proper status.

TRAP_MEMORY:
If YALNIX_ACCERR, print trace and call exit on process with proper status.
If YALNIX_MAPPER, check that offending addr is in region 1, below curent stack allocation,
and above current break. If it is, allocate more stack memory. Otherwise, print trace
and exit with proper status.

TRAP_MATH:
Print trace. Call exit on process with proper status.

TRAP_TTY_RECEIVE:
The corresponding TTY adds the new line buffer. If there are any
procs waiting to receive, it starts with the first one and goes through them. It looks
at their tty_receive_len, builds a new buffer holding min(tty_receive_len, chars in first line buffer) and
puts this min in tty_receive_len for the proc, and points the proc's tty_receive_buf to
this buffer. The TTY then "consumes" what it used to make this buffer, then moves
the buffer off of its waiting to receive queue and adds it to the ready queue.

TRAP_TTY_TRANSMIT:
Look at first proc on waiting to transmit queue. If tty_transmit_len <= TERMINAL_MAX_LINE,
then free tty_transmit_buffer, remove from waiting queue, and move to ready queue, then
transmit for next proc on waiting queue, or return if its empty. Else,
tty_transmit_len -= TERMINAL_MAX_LINE, tty_transmit_pointer += TERMINAL_MAX_LINE,
call TtyTransmit with new len and pointer, and return.

*/

#endif
