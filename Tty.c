#include "Tty.h"

#include <assert.h>

/*
  Initializes allocated memory pointed to by the given Tty *.
*/
void TtyInit(Tty *tty) {
    assert(tty);

    tty->line_buffers = ListNewList();

    tty->waiting_to_receive = ListNewList();

    tty->waitingToTransmit = ListNewList();
}
