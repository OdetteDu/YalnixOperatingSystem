#include <comp421/yalnix.h>
#include <comp421/hardware.h>

void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

struct PhysicalPageNode
{
	  int pageNumber;
	  struct PhysicalPageNode *next;
};

struct PhysicalPageNode *physicalPageNodeHead;
struct PhysicalPageNode *physicalPageNodeCurrent;

struct pte KernelPageTable[PAGE_TABLE_LEN];
struct pte UserPageTable[PAGE_TABLE_LEN];

void allocatePhysicalPage()
{
	  
}

void freePhysicalPage()
{
	  
}

void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapKernel: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapClock(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapClock: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapIllegal(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapIllegal: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapMemory: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
    
	TracePrintf(512, "trapMath: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYReceive: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYTransmit: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

extern int SetKernelBrk(void *addr)
{
	TracePrintf(512, "Set Kernel Brk Called: addr >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)addr >> PAGESHIFT, UP_TO_PAGE(addr), UP_TO_PAGE(addr) >> PAGESHIFT, DOWN_TO_PAGE(addr), DOWN_TO_PAGE(addr) >> PAGESHIFT);
	return 0;
}

extern void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{
	//initialize the interrupt vector Table
	interruptTable[TRAP_KERNEL] = trapKernel;
	interruptTable[TRAP_CLOCK] = trapClock;
	interruptTable[TRAP_ILLEGAL] = trapIllegal;
	interruptTable[TRAP_MEMORY] = trapMemory;
	interruptTable[TRAP_MATH] = trapMath;
	interruptTable[TRAP_TTY_RECEIVE] = trapTTYReceive;
	interruptTable[TRAP_TTY_TRANSMIT] = trapTTYTransmit;
	RCS421RegVal interruptTableAddress = (RCS421RegVal)interruptTable;
	WriteRegister(REG_VECTOR_BASE, interruptTableAddress);
    
    //initialize the physical pages array
	int numOfPagesAvailable = pmem_size/PAGESIZE;
	TracePrintf(1024, "Total number of physical pages: %d\n", numOfPagesAvailable);

    int physicalPages[numOfPagesAvailable];
	int index;
	for ( index=0; index<numOfPagesAvailable; index++)
	{
		physicalPages[index] = 0;
	}
	
	//initialize the page Table
	for( index = 0; index < sizeof(KernelPageTable); index++ )
	{
		  struct pte PTE;
		  PTE.valid = 0;
		  PTE.pfn = 0;
		  PTE.uprot = PROT_NONE;
		  PTE.kprot = PROT_NONE;
		  KernelPageTable[index] = PTE;
	}

	//calculated the existing use of memory

	TracePrintf(1024, "PMEM_BASE: %d, VMEM_BASE: %d, VMEM_0_BASE: %d (%d), VMEM_0_LIMIT: %d (%d), VMEM_1_BASE: %d (%d), VMEM_1_LIMIT: %d (%d)\n", PMEM_BASE, VMEM_BASE, VMEM_0_BASE, VMEM_0_BASE >> PAGESHIFT, VMEM_0_LIMIT, VMEM_0_LIMIT >> PAGESHIFT, VMEM_1_BASE, VMEM_1_BASE >> PAGESHIFT, VMEM_1_LIMIT, VMEM_1_LIMIT >> PAGESHIFT);

	long etextAddr = (long)&_etext;
	TracePrintf(1024, "&_etext: %d, &_etext >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", &_etext, etextAddr >> PAGESHIFT, UP_TO_PAGE(etextAddr), UP_TO_PAGE(etextAddr) >> PAGESHIFT, DOWN_TO_PAGE(etextAddr), DOWN_TO_PAGE(etextAddr) >> PAGESHIFT);

	TracePrintf(1024, "orig_brk: %d, orig_brk >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n",orig_brk, (long)orig_brk >> PAGESHIFT, UP_TO_PAGE(orig_brk), UP_TO_PAGE(orig_brk) >> PAGESHIFT, DOWN_TO_PAGE(orig_brk), DOWN_TO_PAGE(orig_brk) >> PAGESHIFT);

	//assign kernel to page Table
	int limit;
    
    limit = DOWN_TO_PAGE(etextAddr) >> PAGESHIFT;
	for(index = VMEM_1_BASE >> PAGESHIFT; index <= limit; index++)
	{
		struct pte PTE;
		PTE.valid = 1;
		PTE.pfn = index;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_READ | PROT_EXEC;
		KernelPageTable[index] = PTE;
        physicalPages[index] = 1;
	}

	limit = DOWN_TO_PAGE(orig_brk) >> PAGESHIFT;
	for(index = UP_TO_PAGE(etextAddr) >> PAGESHIFT; index <= limit; index++)
	{
		struct pte PTE;
		PTE.valid = 1; 
		PTE.pfn = index;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_READ | PROT_WRITE;
		KernelPageTable[index] = PTE;
        physicalPages[index] = 1;
	}
	
	TracePrintf(4096, "Debugger Break Point: 145\n");
	//use a linked list to store the available physica pages
	physicalPageNodeHead = 0;
	physicalPageNodeCurrent = 0;
	 
	TracePrintf(4096, "Debugger Break Point: 150\n");
	for(index = 1; index < sizeof(physicalPages); index++)
	{
		if(physicalPages[index] == 0)
		{
			TracePrintf(4096, "Debugger Break Point: 155\n");
			//create a linked node
			struct PhysicalPageNode physicalPageNode;
			physicalPageNode.pageNumber = index;
			physicalPageNode.next = 0;

			TracePrintf(4096, "Debugger Break Point: 161\n");
			if(physicalPageNodeHead == 0)
			{
				TracePrintf(4096, "Debugger Break Point: 164\n");
				physicalPageNodeHead = &physicalPageNode;
				physicalPageNodeCurrent = &physicalPageNode;
				TracePrintf(4096, "Debugger Break Point: 167\n");
			}
			else
			{
				physicalPageNodeCurrent -> next = &physicalPageNode;
				physicalPageNodeCurrent = &physicalPageNode;
			}

		}
	}

	//Write the page table address to the register and enable virtual memory
	
}

extern int Fork(void)
{
	TracePrintf(512, "Fork\n");
	return 0;
}

extern int Exec(char *filename, char **argvec)
{
	TracePrintf(512, "Exec: filename(%s), argvec(%s)\n", filename, argvec);
	return 0;
}

extern int Wait(int *status_ptr)
{
	TracePrintf(512, "Wait: status_ptr(%d)\n", *status_ptr);
	return 0;
}

extern int GetPid(void)
{
	TracePrintf(512, "GetPid\n");
	return 0;
}

extern int Brk(void *addr)
{
	TracePrintf(512, "Brk: addr(%d)\n", addr);
	return 0;
}

extern int Delay(int clock_ticks)
{
	TracePrintf(512, "Delay: clock_ticks(%d)\n", clock_ticks);
	return 0;
}

extern int TtyRead(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyRead: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}

extern int TtyWrite(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyWrite: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}




