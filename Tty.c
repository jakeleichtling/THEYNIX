#include "Tty.h"

#include <assert.h>

/*
 * Tty.c
 * Datastructure and helper methods for managing terminals.
 */

/*
  Initializes allocated memory pointed to by the given Tty *.
*/
void TtyInit(Tty *tty) {
    assert(tty);

    // Consumed consecutively, don't need hash
    tty->line_buffers = ListNewList(0);

    // Consumed consecutively, don't need hash
    tty->waiting_to_receive = ListNewList(0);

    // Will look up proc by id when transmit recieves
    tty->waiting_to_transmit = ListNewList(WAITING_TO_TRANSMIT_HASH_SIZE);
}
