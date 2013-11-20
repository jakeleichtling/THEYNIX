/* TRAP_MEMORY: Protection violation */
main()
{
  TracePrintf(0,"about to write\n");
    *(int *)0x100000 = 0;
    TracePrintf(0,"wrote!\n");
    Exit(0);
}
