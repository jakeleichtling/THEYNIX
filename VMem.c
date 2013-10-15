/*
  Mallocs and initializes a region 1 page table with all invalid entries.
*/
void CreateRegion1PageTable(PCB *pcb) {
    unsigned int num_pages = VMEM_1_SIZE / PAGESIZE;
    pcb->region_1_page_table = (struct pte *) malloc(num_pages * sizeof(struct pte));

    unsigned int i;
    for (i = 0; i < num_pages; i++) {
        pcb->region_1_page_table[i].valid = 0;
    }
}
