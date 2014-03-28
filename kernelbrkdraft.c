extern int SetKernelBrk(void *addr)
{
  TracePrintf(512, "Set Kernel Brk Called: addr >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)addr >> PAGESHIFT, UP_TO_PAGE(addr), UP_TO_PAGE(addr) >> PAGESHIFT, DOWN_TO_PAGE(addr), DOWN_TO_PAGE(addr) >> PAGESHIFT);
     
  if(vm_enabled){
	   
    //if(addr>VMEM_1_LIMIT) return -1;
    if(addr > VMEM_1_LIMIT){
      TracePrintf(512, "Error: Kernel heap out of memory");
      return -1;
    }
    struct pte *kernelTablePte;
    //Only allocate for entire pages??
    unsigned int gap = (UP_TO_PAGE(addr)-UP_TO_PAGE(new_brk));
    if(gap>0){
      TracePrintf(512, "Set Kernel Brk Called: moving up");
      for(kernelTablePte = (&KernelPageTable) +(UP_TO_PAGE(new_brk)>>PAGESHIFT); 
	  kernelTablePte< ((&KernelPageTable) +(UP_TO_PAGE(addr)>>PAGESHIFT)); 
	  kernelTablePte ++){
	kernelTablePte->valid = 1;
	kernelTablePte->uprot = PROT_NONE;
	kernelTalbePte->kprot = PROT_READ | PROT_WRITE;
      }
    }else if(gap<0){
      TracePrintf(512, "Set Kernel Brk Called: moving down ");
      for(kernelTablePte = ((&KernelPageTable) +(UP_TO_PAGE(new_brk)>>PAGESHIFT)); 
	  kernelTablePte > ((&KernelPageTable) +(UP_TO_PAGE(addr)>>PAGESHIFT)); 
	  kernelTablePte--){
	kernelTablePte->valid = 1;
	kernelTablePte->uprot = PROT_NONE;
	kernelTalbePte->kprot = PROT_READ | PROT_WRITE;
      }
    }
  }
  new_brk = addr;
  TracePrintf(0, "finish set kernel brk!\n");	
  return 0;
}

