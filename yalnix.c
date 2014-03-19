#include <comp421/yalnix.h>
#include <comp421/hardware.h>

void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

struct PhysicalPageNode
{
	  int pageNumber;
	  struct PhysicalPageNode *next;
};

struct PhysicalPageNode *head;
struct PhysicalPageNode *current;

void allocatePhysicalPage()
{
	  
}

void freePhysicalPage()
{
	  
}

void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapClock(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapIllegal(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
    
}

extern int SetKernelBrk(void *addr)
{
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
	
	//initialize the page Table
	int numOfPagesAvailable = pmem_size/PAGESIZE;
	TracePrintf(1024, "Total number of physical pages: %d\n", numOfPagesAvailable);

	TracePrintf(1024, "PMEM_BASE: %d, VMEM_BASE: %d, VMEM_1_BASE: %d, &_etext: %d, orig_brk: %d\n", PMEM_BASE, VMEM_BASE, VMEM_1_BASE, &_etext, orig_brk);

	TracePrintf(1024, "VMEM_1_BASE >> PAGESHIFT: %d\n", VMEM_1_BASE >> PAGESHIFT);

	long etextAddr = (long)&_etext;
	TracePrintf(1024, "&_etext >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", etextAddr >> PAGESHIFT, UP_TO_PAGE(etextAddr), UP_TO_PAGE(etextAddr) >> PAGESHIFT, DOWN_TO_PAGE(etextAddr), DOWN_TO_PAGE(etextAddr) >> PAGESHIFT);

	TracePrintf(1024, "orig_brk >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)orig_brk >> PAGESHIFT, UP_TO_PAGE(orig_brk), UP_TO_PAGE(orig_brk) >> PAGESHIFT, DOWN_TO_PAGE(orig_brk), DOWN_TO_PAGE(orig_brk) >> PAGESHIFT);

	int i;
	for ( i=0; i<numOfPagesAvailable; i++)
	{
		  
	}
}

extern int Fork(void)
{
	return 0;
}

extern int Exec(char *filename, char **argvec)
{
	return 0;
}

extern void Exit(int status)
{
	
}

extern int Wait(int *status_ptr)
{
	return 0;
}

extern int GetPid(void)
{
	return 0;
}

extern int Brk(void *addr)
{
	return 0;
}

extern int Delay(int clock_ticks)
{
	return 0;
}

extern int TtyRead(int tty_id, void *buf, int len)
{
	return 0;
}

extern int TtyWrite(int tty_id, void *buf, int len)
{
	return 0;
}




