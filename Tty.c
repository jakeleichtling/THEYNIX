/*
  Initializes allocated memory pointed to by the given Tty *.
*/
void TtyInit(Tty *tty) {
  line_buffers = NewList();

  waiting_to_receive = NewList();

  waitingToTransmit = NewList();
}
