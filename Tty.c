#include "Tty.h"

#include <assert.h>

/*
  Initializes allocated memory pointed to by the given Tty *.
*/
void TtyInit(Tty *tty) {
    assert(tty);

    tty->line_buffers = ListNewList(0);

    tty->waiting_to_receive = ListNewList(0);

    tty->waiting_to_transmit = ListNewList(WAITING_TO_TRANSMIT_HASH_SIZE);
}
